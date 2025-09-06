#include "io/vga_control.h"
#include "mem/alloc_init.h"
#include "mem/memutils.h"
#include "interrupts/idt.h"
#include "paging/paging.h"
#include <stdint.h>



struct kern_data{
    struct idt_ptr idt_pointer;
    struct pagetable* pml4;
};