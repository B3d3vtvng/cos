#include "vmalloc.h"
#include <stdint.h> 

#define PAGE_SHIFT 12
#define PAGE_SIZE (1UL << PAGE_SHIFT)
#define PAGE_MASK (~(PAGE_SIZE - 1))
#define MAX_ORDER 12

struct free_list {
    struct free_list *next;
};

static struct free_list *free_lists[MAX_ORDER];
static spinlock_t vmalloc_lock = SPINLOCK_INIT;

/* * Global pointer to our metadata array.
 * page_orders[i] stores the 'order' of the allocation starting at page i.
 */
static uint8_t *page_orders = NULL;
static uint64_t heap_base_addr = 0;

#define ALIGN_UP(addr) (((addr) + PAGE_SIZE - 1) & PAGE_MASK)
#define ALIGN_DOWN(addr) ((addr) & PAGE_MASK)

void vmalloc_init(void)
{
    uint64_t start = ALIGN_UP(VMALLOC_BASE);
    uint64_t end   = ALIGN_DOWN(VMALLOC_MAX);
    uint64_t total_size = end - start;
    uint64_t total_pages = total_size >> PAGE_SHIFT;

    if (start >= end) return;

    uint64_t metadata_bytes = total_pages;
    uint64_t metadata_pages = (metadata_bytes + PAGE_SIZE - 1) >> PAGE_SHIFT;

    for (uint64_t i = 0; i < metadata_pages; i++) {
        map_virtual(get_pgtable(),
                    start + i * PAGE_SIZE,
                    (uint64_t)pmmalloc(1),
                    P_PRESENT | P_WRITABLE | P_KERNEL | P_NX_ENABLE,
                    PG_SIZE_REG);
    }

    page_orders = (uint8_t *)start;

    for (uint64_t i = 0; i < total_pages; i++) {
        page_orders[i] = 0xFF;
    }

    start += metadata_pages * PAGE_SIZE;
    heap_base_addr = start;

    for (int i = 0; i < MAX_ORDER; i++) {
        free_lists[i] = NULL;
    }

    for (int order = MAX_ORDER - 1; order >= 0; order--) {
        uintptr_t block_size = 1UL << (order + PAGE_SHIFT);

        while (start + block_size <= end) {
            map_virtual(get_pgtable(),
                        start,
                        (uint64_t)pmmalloc(1),
                        P_PRESENT | P_WRITABLE | P_KERNEL | P_NX_ENABLE,
                        PG_SIZE_REG);

            struct free_list *block = (struct free_list *)start;
            block->next = free_lists[order];
            free_lists[order] = block;

            start += block_size;
        }
    }
}

//Allocates 'pages' number of contiguous virtual pages
void *alloc_virt(size_t pages) {
    if (pages <= 0) return NULL;

    // Calculate total size needed (Power of 2)
    uint64_t needed_size = (uint64_t)pages * PAGE_SIZE;
    
    int order = 0;
    while (order < MAX_ORDER) {
        if ((1UL << (order + PAGE_SHIFT)) >= needed_size) {
            break;
        }
        order++;
    }

    if (order >= MAX_ORDER)
        kernel_panic(NULL, "vmalloc: request too large");

    flags_t flags = spinlock_aquire(&vmalloc_lock);

    for (int i = order; i < MAX_ORDER; i++) {
        if (free_lists[i]) {
            struct free_list *block = free_lists[i];
            free_lists[i] = block->next;

            while (i > order) {
                i--;
                uintptr_t buddy_addr = (uintptr_t)block + (1UL << (i + PAGE_SHIFT));
                struct free_list *buddy = (struct free_list *)buddy_addr;
                buddy->next = free_lists[i];
                free_lists[i] = buddy;
            }

            // --- Metadata Handling ---
            // Calculate the index of this page relative to the heap base
            uint64_t page_idx = ((uintptr_t)block - heap_base_addr) >> PAGE_SHIFT;
            
            // Store the order in our side-array
            page_orders[page_idx] = (uint8_t)order;
            
            spinlock_release(&vmalloc_lock, flags);
            return block; // Pointer is perfectly page-aligned
        }
    }

    spinlock_release(&vmalloc_lock, flags);
    kernel_panic(NULL, "vmalloc: OOM");
    __builtin_unreachable();
}

void free_virt(void *ptr) {
    if (!ptr) return;

    uintptr_t addr = (uintptr_t)ptr;

    // Check alignment
    if (addr & (PAGE_SIZE - 1))
        kernel_panic(NULL, "vfree: unaligned pointer");

    // Check bounds
    if (addr < heap_base_addr || addr >= VMALLOC_MAX)
        kernel_panic(NULL, "vfree: pointer out of bounds");

    // --- Metadata Retrieval ---
    // 1. Calculate index
    uint64_t page_idx = (addr - heap_base_addr) >> PAGE_SHIFT;

    // 2. Lookup the order from our side-array
    int order = (int)page_orders[page_idx];

    if (order >= MAX_ORDER || order < 0) 
        kernel_panic(NULL, "vfree: double free or corruption");

    // OPTIONAL: Mark as freed in metadata to detect double-frees later
    page_orders[page_idx] = 0xFF; 

    flags_t flags = spinlock_aquire(&vmalloc_lock);

    while (order < MAX_ORDER - 1) {
        uintptr_t block_size = 1UL << (order + PAGE_SHIFT);
        uintptr_t buddy_addr = addr ^ block_size;
        
        struct free_list **p = &free_lists[order];
        struct free_list *buddy = NULL;

        while (*p) {
            if ((uintptr_t)*p == buddy_addr) {
                buddy = *p;
                *p = buddy->next;
                break;
            }
            p = &(*p)->next;
        }

        if (!buddy) break;

        addr = addr < buddy_addr ? addr : buddy_addr;
        order++;
    }

    struct free_list *block = (struct free_list *)addr;
    block->next = free_lists[order];
    free_lists[order] = block;

    spinlock_release(&vmalloc_lock, flags);
}

// Returns the size of the allocation in pages
uint64_t get_allocation_size(void* addr){
    if (!addr) return 0;

    uintptr_t address = (uintptr_t)addr;

    // Check alignment
    if (address & (PAGE_SIZE - 1))
        return 0;

    // Check bounds
    if (address < heap_base_addr || address >= VMALLOC_MAX)
        return 0;

    // --- Metadata Retrieval ---
    // 1. Calculate index
    uint64_t page_idx = (address - heap_base_addr) >> PAGE_SHIFT;

    // 2. Lookup the order from our side-array
    int order = (int)page_orders[page_idx];

    if (order >= MAX_ORDER || order < 0) 
        return 0;

    return (1UL << (order));
}

// Allocates 'pages' number of contiguous virtual pages, backed by potentially non-contiguous physical pages, mapped with the provided protection flags
// 'prot_flags' uses the same bit definitions as page table entries (P_PRESENT, P_WRITABLE, etc.)
void* vmalloc(size_t pages, uint64_t prot_flags){
    void* first = NULL;

    for (size_t i = 0; i < pages; i++){
        void* vpage = alloc_virt(1);
        if (first == NULL){
            first = vpage;
        }
        void* ppage = pmmalloc(1);
        map_virtual(get_pgtable(), (uint64_t)vpage, (uint64_t)ppage, prot_flags, PG_SIZE_REG);
    }

    return first;
}

// Frees a virtual allocation made by vmalloc
// Also unmaps the pages and frees the underlying physical pages
void vfree(void* ptr){
    if (ptr == NULL) return;
    if (virt_to_phys((uint64_t)ptr) == 0) return;

    uint64_t size = get_allocation_size(ptr);
    if (size == 0) return;

    for (int i = 0; i < size; i++){
        uint64_t phys = virt_to_phys((uint64_t)ptr + i * PAGE_SIZE);
        if (phys == 0) continue;

        pmmfree((void*)phys);
        unmap_virtual(get_pgtable(), (uint64_t)ptr + i * PAGE_SIZE, PG_SIZE_REG);
    }

    return free_virt(ptr);
}