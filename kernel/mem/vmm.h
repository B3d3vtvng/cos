#ifndef VMM_H
#define VMM_H

#define ENTRIES_PER_TABLE 512
#define P_PRESENT (1ULL << 0)
#define P_WRITABLE (1ULL << 1)
#define P_READONLY (0ULL << 1)
#define P_USER (1ULL << 2)
#define P_KERNEL (0 << 2)
#define P_NX_ENABLE (1ULL << 63)
#define P_PS (1ULL << 7)

#define PAGE_SIZE 0x1000

#define RAM_MAP_BASE

#include "stdbool.h"

#include "vmalloc.h"
#include "../kernel.h"
#include "pmmalloc.h"
#include "paging.h"
#include "kmalloc.h"
#include "../util/string.h"
#include "../sched/gdt.h"

void vmm_init(struct gdt_ptr* gdt_ptr);
void* vpgalloc(size_t);
void vpgfree(void*);

#endif