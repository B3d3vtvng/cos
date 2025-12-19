#include "pmmalloc.h"

// Usable memory map entries and count
static struct bios_mmap_entry usable_entries[64];
static size_t usable_entry_count;
// Global metadata structure
struct alloc_metadata alloc_meta;
// Total number of pages used for the metadata structure itself
static uint64_t metadata_page_count;
static spinlock_t pmmalloc_lock = SPINLOCK_INIT;

//Store the highest physical address for the vmm ram map
static uint64_t mem_max;

void pmm_switch_virt(void){
    alloc_meta.alloc_entries = (struct pg_entry*)((uintptr_t) alloc_meta.alloc_entries | (uintptr_t) 0xFFFF800000000000ULL);
}

uint64_t get_mem_max(void){ return mem_max; }

// Helper function definitions
uint64_t round_page_down(uint64_t addr);
static uint64_t get_page_index(uint64_t addr);
uint64_t get_page_addr(uint64_t index);

// Function to find the global page index for a given physical address
static uint64_t get_page_index(uint64_t addr) {
    uint64_t current_page_index = 0;
    
    // Check for page alignment
    if (addr % PAGE_SZ != 0) return (uint64_t)-1;

    for (size_t i = 0; i < usable_entry_count; i++) {
        uint64_t base = usable_entries[i].base_addr;
        uint64_t top = base + usable_entries[i].length;
        
        // Check if the address is within the current usable entry
        if (addr >= base && addr < top) {
            uint64_t offset = addr - base;
            // Calculate index within this entry and add it to the cumulative index
            return current_page_index + (offset / PAGE_SZ);
        }

        // Address not found in this entry, add its page count to the index
        current_page_index += round_page_down(usable_entries[i].length) / PAGE_SZ;
    }

    return (uint64_t)-1; // Address out of bounds
}

// Function to find the physical address for a given global page index
uint64_t get_page_addr(uint64_t index) {
    uint64_t remaining_index = index;

    for (size_t i = 0; i < usable_entry_count; i++) {
        uint64_t entry_page_count = round_page_down(usable_entries[i].length) / PAGE_SZ;
        
        if (remaining_index < entry_page_count) {
            // Found the correct entry. Calculate the address.
            return usable_entries[i].base_addr + (remaining_index * PAGE_SZ);
        }

        // Subtract the page count of this entry and continue
        remaining_index -= entry_page_count;
    }

    return 0; // Index out of bounds
}

void* pmmalloc(unsigned long count){
    // Count is uint64_t in the new pg_entry, but we cast to unsigned long for consistency
    if (count == 0 || count > KPG_MALLOC_MAX) {
        return (void*)0; // Invalid size
    }

    uint64_t contiguous = 0;
    uint64_t start_index = 0;

    flags_t flags = spinlock_aquire(&pmmalloc_lock);
    
    // Loop over the total number of allocatable pages
    for (uint64_t i = 0; i < alloc_meta.alloc_entry_count; i++) {
        if (alloc_meta.alloc_entries[i].used == 0) {
            if (contiguous == 0) {
                start_index = i; // Mark the beginning of a free run
            }
            contiguous++;
            if (contiguous == count) {
                // Mark pages as used
                for (uint64_t j = 0; j < count; j++) {
                    alloc_meta.alloc_entries[start_index + j].used = 1;
                    // Only the first page needs to store the total count
                    alloc_meta.alloc_entries[start_index + j].contiguous_count = (j == 0) ? count : 0;
                }

                // Convert the global page index back to a physical address
                uint64_t addr = get_page_addr(start_index);
                
                if (addr == 0) {
                    // This should not happen if indexing is correct
                    spinlock_release(&pmmalloc_lock, flags);
                    kernel_panic(NULL, "PMMALLOC: Internal index-to-address conversion error");
                }
                
                spinlock_release(&pmmalloc_lock, flags);
                return (void*)(addr);
            }
        } 
        else {
            contiguous = 0; // Reset count on encountering a used page
        }
    }

    spinlock_release(&pmmalloc_lock, flags);
    return (void*)0; // No sufficient memory
}

void pmmfree(void* ptr) {
    if (ptr == NULL){
        return; // Invalid pointer
    }

    uint64_t addr = (uint64_t)ptr;
    
    // Get the global page index for the address
    uint64_t index = get_page_index(addr);

    if (index == (uint64_t)-1 || index >= alloc_meta.alloc_entry_count) {
        return; // Not page-aligned or out of bounds
    }

    if (alloc_meta.alloc_entries[index].used == 0) {
        return; // Double free or invalid free (not marked as used)
    }

    uint64_t count = alloc_meta.alloc_entries[index].contiguous_count;
    
    // Check if this is the start of an allocated block (must have a non-zero count)
    if (count == 0 || index + count > alloc_meta.alloc_entry_count) {
        return; // Invalid free: pointer not to the start of a block
    }
    
    flags_t flags = spinlock_aquire(&pmmalloc_lock);

    // Mark the block as free
    for (uint64_t i = 0; i < count; i++) {
        alloc_meta.alloc_entries[index + i].used = 0;
        alloc_meta.alloc_entries[index + i].contiguous_count = 0;
    }

    spinlock_release(&pmmalloc_lock, flags);
}

// Round down to nearest page boundary
uint64_t round_page_down(uint64_t addr) {
    return addr & ~(PAGE_SZ - 1);
}

// Round up to nearest page boundary
uint64_t round_page_up(uint64_t addr) {
    return (addr + PAGE_SZ - 1) & ~(PAGE_SZ - 1);
}

size_t get_kernel_pg_count(void);

void mark_used(void){
    int pgtable_cnt = *PGTABLE_CNT;

    int pml4_index = get_page_index(0x100000);
    for (int i = pml4_index; i < pml4_index + pgtable_cnt; i++){
        alloc_meta.alloc_entries[i].used = 1;
        alloc_meta.alloc_entries[i].contiguous_count = 1;
    }

    int kstack_index = get_page_index(0x10000);
    alloc_meta.alloc_entries[kstack_index].contiguous_count = 9;
    for (int i = kstack_index; i < kstack_index + 9; i++){
        alloc_meta.alloc_entries[i].used = 1;
    }

    int k_index = get_page_index(0x20000);
    int k_pg_cnt = get_kernel_pg_count();
    alloc_meta.alloc_entries[k_index].contiguous_count = k_pg_cnt;
    for (int i = k_index; i < k_index + k_pg_cnt; i++){
        alloc_meta.alloc_entries[i].used = 1;
    }

    return;
}

void init_pmm(void) {
    int bios_mmap_count = BIOS_MMAP_COUNT;
    uint64_t total_pages = 0;
    struct bios_mmap_entry* bios_mmap = BIOS_MMAP_ADDR;

    // 1. Populate usable_entries list and count total pages
    for (int i = 0; i < bios_mmap_count; i++) {
        // Type 1 indicates usable RAM, and skip the first 640KB / the low memory region (< 0x10000)
        // Check base_addr < 0x10000 is to ensure the allocated metadata is included in mapping
        if (bios_mmap[i].type == 1 && bios_mmap[i].base_addr < 0x10000) {
             // If usable memory is below 0x10000 but extends past it, split the entry
             if (bios_mmap[i].base_addr + bios_mmap[i].length > 0x10000) {
                uint64_t new_base = 0x10000;
                uint64_t new_length = (bios_mmap[i].base_addr + bios_mmap[i].length) - new_base;
                bios_mmap[i].length = 0; // Zero out the original entry

                // Add the split entry starting from 0x10000
                usable_entries[usable_entry_count].base_addr = new_base;
                usable_entries[usable_entry_count].length = new_length;
                usable_entries[usable_entry_count++].type = 1;
                
                total_pages += round_page_down(new_length) / PAGE_SZ;
             }
        } 
        else if (bios_mmap[i].type == 1 && bios_mmap[i].base_addr >= 0x10000) {
            // Usable RAM is above 0x10000 (normal case)
            uint64_t pages_in_entry = round_page_down(bios_mmap[i].length) / PAGE_SZ;
            
            usable_entries[usable_entry_count].base_addr = bios_mmap[i].base_addr;
            usable_entries[usable_entry_count].length = bios_mmap[i].length;
            usable_entries[usable_entry_count++].type = bios_mmap[i].type;

            total_pages += pages_in_entry;
        }
    }

    mem_max = usable_entries[usable_entry_count-1].base_addr + usable_entries[usable_entry_count-1].length;
    
    // The total number of allocatable pages
    alloc_meta.alloc_entry_count = total_pages;

    // 2. Calculate metadata size and initialize the entries
    // Size needed = total_pages * sizeof(struct pg_entry)
    uint64_t metadata_size = total_pages * sizeof(struct pg_entry);
    metadata_page_count = round_page_up(metadata_size) / PAGE_SZ;

    // The start of the entries array
    alloc_meta.alloc_entries = (struct pg_entry*)ALLOC_ENTRY_BASE;

    // Zero out the entire metadata region
    for (uint64_t i = 0; i < metadata_page_count * PAGE_SZ; i++){
        ((uint8_t*)alloc_meta.alloc_entries)[i] = 0;
    }

    // 3. Mark the pages used by the metadata itself as used (from index 0)
    // The metadata starts at 0x10000, which corresponds to the first page index (0) 
    // *if* 0x10000 is the start of the first usable entry.
    uint64_t metadata_start_index = get_page_index((uint64_t)ALLOC_ENTRY_BASE);
    
    if (metadata_start_index == (uint64_t)-1 || metadata_start_index != 0) {
        // This is a critical error: metadata is not at the start of the index space
        kernel_panic(NULL, "PMMALLOC: Metadata must start at global page index 0");
    }

    for (uint64_t i = metadata_start_index; i < metadata_start_index + metadata_page_count; i++){
        alloc_meta.alloc_entries[i].used = 1;
        // Mark the first page of the block with the total count
        alloc_meta.alloc_entries[i].contiguous_count = (i == metadata_start_index) ? metadata_page_count : 0;
    }

    int pg_count = 0;
    for (int i = 0; i < alloc_meta.alloc_entry_count; i++){
        if (!alloc_meta.alloc_entries[i].used) pg_count++;
    }

    mark_used();

    vga_print("Initialized physical memory allocator\n");
    kprintf("Found %d usable pages (%d KB)\n\n", pg_count, (pg_count * 4096) / 1000);

    return;
}