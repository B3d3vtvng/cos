#ifndef VALLOC_H
#define VALLOC_H

#define VMALLOC_BASE 0xFFFFFFFF90000000ULL
#define VMALLOC_MAX 0xFFFFFFFF90F00000ULL

#include <stdint.h>
#include <stddef.h>

#include "../sched/spinlock.h"
#include "../idt/idt.h"
#include "paging.h"

void* vmalloc(size_t pages, uint64_t prot_flags);
void vfree(void*);
void* alloc_virt(size_t);
void free_virt(void*);
void vmalloc_init(void);

#endif;