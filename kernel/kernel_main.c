#include "kernel.h"

struct kernel_metadata kernel_meta;

__attribute__((noreturn)) __attribute__((naked)) void __hlt(void) {
    while (1) {
        __asm__ volatile ("hlt");
    }
}

__attribute__((noreturn)) __attribute__((section(".text.kern_entry"))) void kernel_main(void) {
    // Enable interrupts as were now fairly safe
    __asm__ __volatile__ (
        "sti"
    );

    kernel_meta.pagetable_address = PAGETABLE_ADDRESS;

    vga_clear();
    vga_print("Welcome to COS Kernel!\n");

    // Initialize physical memory manager
    init_pmm();
    kernel_meta.idt_ptr = idt_init();
    vga_print("IDT initialized.\n");

    //Test: Trigger a kernel panic
    __asm__ __volatile__("ud2");

    __hlt();
}