#ifndef PMMALLOC_H
#define PMMALLOC_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096  // 4KB pages
#define BIOS_MM_ADDR (void*)0x7504
#define BIOS_MM_SIZE_ADDR (void*)0x7500

void* pmmalloc(size_t num_pages);
void pmmfree(void* ptr, size_t num_pages);

struct bios_mm_entry{
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t acpi_ext;
} __attribute__((packed));

struct pmmpg_state{
    uint8_t* free_page_bitmap;
    uint8_t* alloc_pg_count;
    size_t total_pages;
};

#endif