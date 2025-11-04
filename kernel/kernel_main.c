#include "kernel.h"

__attribute__((noreturn)) __attribute__((naked)) void __hlt(void) {
    while (1) {
        __asm__ volatile ("hlt");
    }
}

__attribute__((noreturn)) __attribute__((section(".text.kern_entry"))) void kernel_main(void) {
    vga_clear();
    vga_print("Welcome to COS Kernel!\n");

    __hlt();
}