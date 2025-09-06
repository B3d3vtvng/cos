#include "kernel.h"

void __attribute__((noreturn)) enter_long_mode(void){
    __asm__ __volatile__ (
        "mov ecx, 0xC0000080"
        "rdmsr"
        "or  eax, 1 << 8"
        "wrmsr"

        "mov eax, cr0"
        "or eax, 1 << 31"
        "mov cr0, eax"

    )
}

void __attribute__((noreturn)) kmain(void) {
    vga_print("Reached kernel main, now enabling virtualization and 64-bit long mode...\n");

    struct idt_ptr idt_pointer = idt_init();
    int x = 1/0;
    init_paging();
    enter_long_mode();

    //Should never be reached
    while (1) {
        __asm__ __volatile__ ("hlt");
    }
}