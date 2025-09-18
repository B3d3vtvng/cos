#ifndef __KERNEL_H__
#define __KERNEL_H__

#include "vga_control.h"
#include "alloc_init.h"
#include "memutils.h"
#include "paging.h"
#include "../../kernel64/include/idt.h"
#include <stdint.h>

struct kern_data{
    struct idt_ptr idt_pointer;
    struct pagetable* pml4;
};

#endif