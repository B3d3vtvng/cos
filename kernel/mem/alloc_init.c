#include "alloc_init.h"
#include "../io/conversion.h"

static struct bios_mmap_entry usable_entries[64];
static struct alloc_metadata alloc_meta;

void* kpgmalloc(unsigned long count){
    if (count == 0 || count > KPG_MALLOC_MAX) {
        return (void*)0; // Invalid size
    }

    //Find contiguous free pages
    uint64_t contiguous = 0;
    uint64_t start_index = 0;
    for (uint64_t i = 0; i < alloc_meta.alloc_entry_count * 8; i++) {
        if (alloc_meta.alloc_entries[i].used == 0) {
            if (contiguous == 0) {
                start_index = i;
            }
            contiguous++;
            if (contiguous == count) {
                //Mark pages as used
                for (uint64_t j = 0; j < count; j++) {
                    alloc_meta.alloc_entries[start_index + j].used = 1;
                    alloc_meta.alloc_entries[start_index + j].contiguous_count = (j == 0) ? count : 0;
                }
                //Store allocation size just before the returned pointer
                uint64_t addr = (uint64_t)ALLOC_BASE + start_index * PAGE_SZ;
                return (void*)(addr);
            }
        } 
        else {
            contiguous = 0;
        }
    }

    return (void*)0; // No sufficient memory
}

void kpgfree(void* ptr) {
    if (ptr == (void*)0 || (uint64_t)ptr < (uint64_t)ALLOC_BASE) {
        return; // Invalid pointer
    }

    uint64_t addr = (uint64_t)ptr;
    if ((addr - (uint64_t)ALLOC_BASE) % PAGE_SZ != 0) {
        return; // Not page-aligned
    }

    uint64_t index = (addr - (uint64_t)ALLOC_BASE) / PAGE_SZ;
    if (index >= alloc_meta.alloc_entry_count * 8) {
        return; // Out of bounds
    }

    if (alloc_meta.alloc_entries[index].used == 0) {
        return; // Double free or invalid free
    }

    uint64_t count = alloc_meta.alloc_entries[index].contiguous_count;
    for (uint64_t i = 0; i < count; i++) {
        alloc_meta.alloc_entries[index + i].used = 0;
        alloc_meta.alloc_entries[index + i].contiguous_count = 0;
    }
}

static inline uint64_t round_page_down(uint64_t addr) {
    return addr & ~(PAGE_SZ - 1);
}

// Round up to nearest page
static inline uint64_t round_page_up(uint64_t addr) {
    return (addr + PAGE_SZ - 1) & ~(PAGE_SZ - 1);
}

void print_usable(uint64_t total_mem){
    vga_print("Total usable memory: ");
    char mem_str[20];
    uint_to_str((unsigned int)(total_mem / (1024 * 1024)), mem_str, 10);
    vga_print(mem_str);
    vga_print(" MB\n");
}

void* init_allocator(void) {
    int bios_mmap_count = BIOS_MMAP_COUNT;

    uint64_t total_mem = 0;
    struct bios_mmap_entry* bios_mmap = BIOS_MMAP_ADDR;

    int usable_entry_count = 0;

    for (int i = 0; i < bios_mmap_count; i++) {
        if (bios_mmap[i].type == 1 && bios_mmap[i].base_addr >= 0x10000) { // Type 1 indicates usable RAM
            total_mem += bios_mmap[i].length;
            usable_entries[usable_entry_count++].base_addr = bios_mmap[i].base_addr;
            usable_entries[usable_entry_count - 1].length = bios_mmap[i].length;
            usable_entries[usable_entry_count - 1].type = bios_mmap[i].type;
        }
    }

    total_mem -= 0x1000; //Account for alloc btmap in 0x10000 to 0x11000

    vga_print("Loaded BIOS memory map\n");
    print_usable(total_mem);

    alloc_meta.alloc_entries = (struct pg_entry*)ALLOC_ENTRY_BASE;
    memset(alloc_meta.alloc_entries, 0, PAGE_SZ); // Clear bitmap area
    alloc_meta.alloc_entry_count = round_page_down(total_mem) / PAGE_SZ / 8; // Each entry is 1 byte

    vga_print("Initialized memory allocator: ");
    char btmap_sz_str[20];
    uint_to_str((unsigned int)alloc_meta.alloc_entry_count, btmap_sz_str, 10);
    vga_print(btmap_sz_str);
    vga_print(" usable pages\n");

    void* stack_ptr = kpgmalloc(KERNEL_STACK_PG_COUNT);

    if (stack_ptr == (void*)0) {
        vga_print("Failed to allocate kernel stack\n");
        return 0;
    }

    return stack_ptr + (KERNEL_STACK_PG_COUNT * PAGE_SZ);
}