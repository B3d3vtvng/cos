#include "kernel64/include/vga_control.h"
#include "kernel64/include/alloc.h"
#include "kernel64/include/memutils.h"
#include "kernel64/include/idt.h"
#include "kernel32/include/paging.h"
#include <stdint.h>

struct kern_data{
    struct idt_ptr idt_pointer;
    struct pagetable* pml4;
};