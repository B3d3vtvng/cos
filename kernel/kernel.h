#ifndef KERNEL_H
#define KERNEL_H

#include "drivers/vga.h"
#include "idt/idt.h"
#include "mem/pmmalloc.h"
#include "util/conversion.h"
#include "sched/gdt.h"
#include "mem/vmm.h"
#include <stdint.h>

#define PAGETABLE_ADDRESS (void*)0x90000

struct kernel_metadata {
    void* pagetable_address;
    struct idt_ptr idt_ptr;
};

#endif