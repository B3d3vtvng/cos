#ifndef VMMALLOC_H
#define VMMALLOC_H

#define ENTRIES_PER_TABLE 512
#define P_PRESENT (1ULL << 0)
#define P_WRITABLE (1ULL << 1)
#define P_USER (1ULL << 2)
#define P_KERNEL (0 << 2)
#define P_NX_ENABLE (1ULL << 63)

#include "../kernel.h"
#include "pmmalloc.h"
#include "../sched/paging.h"

void* vmmalloc(size_t);
void vmmfree(void*);
struct pagetable* vmm_init();

#endif