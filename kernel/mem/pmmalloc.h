#ifndef PMMALLOC_H
#define ALLOC_CONTROL_H

#include <stdint.h>

#include "../drivers/vga.h"

#define BIOS_MMAP_ADDR      ((struct bios_mmap_entry*)0x7504)
#define BIOS_MMAP_COUNT    (*(int*)0x7500)
#define ALLOC_BASE ((void*)0x60000)
#define ALLOC_ENTRY_BASE ((void*)0x10000)
#define PAGE_SZ 0x1000ULL
#define KPG_MALLOC_MAX 128
#define KERNEL_STACK_PG_COUNT 5

struct bios_mmap_entry {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
} __attribute__((packed));

struct pg_entry {
    int used : 1;
    int contiguous_count : 7; // Max 128 contiguous pages
} __attribute__((packed));

struct alloc_metadata {
    struct pg_entry* alloc_entries;
    uint64_t alloc_entry_count;
};

void init_pmm(void);
void* pmmalloc(unsigned long size);
void pmmfree(void* ptr);

#endif