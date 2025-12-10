#ifndef PMMALLOC_H
#define PMMALLOC_H

#include <stdint.h>
#include <stddef.h>

#include "../drivers/vga.h"
#include "../util/printf.h"
#include "../util/conversion.h"
#include "../idt/idt.h"

#define BIOS_MMAP_ADDR      ((struct bios_mmap_entry*)0x7504)
#define BIOS_MMAP_COUNT    (*(int*)0x7500)
#define ALLOC_ENTRY_BASE ((void*)0x10000)
#define PAGE_SZ 0x1000ULL
#define KPG_MALLOC_MAX 128 // Max allocation size is 127 pages
#define PGTABLE_CNT ((int*)0x74FB)
// A struct to track the usable memory map entries
struct bios_mmap_entry {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
} __attribute__((packed));

struct pg_entry {
    uint8_t used : 1;
    uint8_t contiguous_count : 7;
} __attribute__((packed));

// The allocation metadata structure
struct alloc_metadata {
    struct pg_entry* alloc_entries;
    uint64_t alloc_entry_count; // Total number of allocatable pages
};

void init_pmm(void);
void* pmmalloc(unsigned long count);
void pmmfree(void* ptr);
uint64_t round_page_down(uint64_t);
uint64_t round_page_up(uint64_t);
uint64_t get_mem_max(void);

#endif // PMMALLOC_H