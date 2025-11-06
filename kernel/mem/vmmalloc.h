#ifndef VMMALLOC_H
#define VMMALLOC_H

#define KERN_BASE_VADDR 0xFFFF800000000000ULL
#define USER_BASE_VADDR 0x0000000000000000ULL

#include "../kernel.h"
#include "pmmalloc.h"
#include "../sched/paging.h"

void* vmmalloc(size_t);
void vmmfree(void*);
struct pagetable* vmm_init();

#endif