#include "../include/alloc_init.h"

static struct alloc_metadata alloc_meta;

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