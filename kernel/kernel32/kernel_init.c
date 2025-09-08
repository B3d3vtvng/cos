#include "../include/kernel.h"

extern void __attribute__((noreturn)) long_mode_jmp(void);

//###########################
// GDT Setup
//###########################

struct gdt_entry {
    uint16_t limit_low;     // Bits 0-15 of limit
    uint16_t base_low;      // Bits 0-15 of base
    uint8_t  base_middle;   // Bits 16-23 of base
    uint8_t  access;        // Access flags
    uint8_t  granularity;   // Flags + bits 16-19 of limit
    uint8_t  base_high;     // Bits 24-31 of base
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint64_t base;          // 64-bit base for lgdt
} __attribute__((packed));

struct gdt_entry gdt64[3];
struct gdt_ptr gdt64_ptr;

void gdt_set_entry(int idx, uint32_t base, uint32_t limit,
                   uint8_t access, uint8_t gran) {
    gdt64[idx].limit_low    = (limit & 0xFFFF);
    gdt64[idx].base_low     = (base & 0xFFFF);
    gdt64[idx].base_middle  = (base >> 16) & 0xFF;
    gdt64[idx].access       = access;
    gdt64[idx].granularity  = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt64[idx].base_high    = (base >> 24) & 0xFF;
}

void init_gdt64() {
    // Null descriptor
    gdt_set_entry(0, 0, 0, 0, 0);

    // Code: base=0, limit=0, Access=0x9A, Flags=0x20 (L-bit)
    gdt_set_entry(1, 0, 0, 0x9A, 0x20);

    // Data: base=0, limit=0, Access=0x92, Flags=0x00
    gdt_set_entry(2, 0, 0, 0x92, 0x00);

    gdt64_ptr.limit = sizeof(gdt64) - 1;
    gdt64_ptr.base  = (uint64_t)&gdt64;
}

//###########################
// Initialization
//###########################

struct kern_data kernel_dat;

void __attribute__((noreturn)) enter_long_mode(void){
    __asm__ __volatile__ (
    // Enable LME (long mode enable) in IA32_EFER (MSR 0xC0000080)
    "mov $0xC0000080, %%ecx\n\t"
    "rdmsr\n\t"
    "or $(1 << 8), %%eax\n\t"
    "wrmsr\n\t"

    // Enable paging (PG bit in CR0)
    "mov %%cr0, %%eax\n\t"
    "or $(1 << 31), %%eax\n\t"
    "mov %%eax, %%cr0\n\t"
    :
    :
    : "eax", "ecx", "edx"
);

    init_gdt64();
    long_mode_jmp();

    __builtin_unreachable();
}

void __attribute__((noreturn)) kinit(void) {
    vga_print("Reached kernel main, now enabling virtualization and 64-bit long mode...\n");

    kernel_dat.pml4 = init_paging();

    enter_long_mode();

    __builtin_unreachable();
}