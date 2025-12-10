#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stddef.h>

#include "pmmalloc.h"
#include "vpgalloc.h"

#define ENTRIES_PER_TABLE 512
#define P_PRESENT (1ULL << 0)
#define P_WRITABLE (1ULL << 1)
#define P_READONLY (0ULL << 1)
#define P_USER (1ULL << 2)
#define P_KERNEL (0 << 2)
#define P_NX_ENABLE (1ULL << 63)
#define P_PS (1ULL << 7)
#define P_PCD (1ULL << 4)
#define P_PWT   (1ULL << 3)
#define P_PCD   (1ULL << 4)
#define P_PAT   (1ULL << 7)

#define P_WC    (P_PAT | P_PWT)

enum pg_size {
    PG_SIZE_REG,
    PG_SIZE_LARGE,
    PG_SIZE_HUGE
};

typedef uint64_t pte_t;

struct pagetable {
    pte_t entries[ENTRIES_PER_TABLE];
};

void set_paging_allocator(void* (*alloc_func)(size_t));
void set_paging_free(void (*free_func)(void*));
uint64_t get_cr2(void);
void invtlb(void);
void invlpg(uint64_t addr);
uint64_t* get_pg_entry(struct pagetable* pml4, uint64_t vaddr);
int set_pagetable_entry(struct pagetable* pml4, uint64_t vaddr, uint64_t val, int flush);
int set_pagetable_prot(struct pagetable*, uint64_t vaddr, uint64_t flags, int flush);
struct pagetable* get_pgtable(void);
void set_pgtable(struct pagetable* pgtable);
void unmap_virtual(struct pagetable* pml4, uint64_t virt, enum pg_size pg_size);
int map_virtual(struct pagetable* pml4, uint64_t virt, uint64_t phys, uint64_t flags, enum pg_size pg_size);
uint64_t virt_to_phys(uint64_t vaddr);

#endif