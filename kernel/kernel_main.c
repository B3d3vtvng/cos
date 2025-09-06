#include "io/vga_control.h"
#include "mem/alloc_init.h"
#include "mem/memutils.h"
#include "interrupts/idt.h"
#include <stdint.h>

void __attribute__((noreturn)) kmain(void) {
    vga_print("Reached kernel main, now enabling virtualization and 64-bit long mode...\n");

    struct idt_ptr idt_pointer = idt_init();
    int x = 1/0;
    //enable_virt();
    //enter_long_mode();
    while (1) {
        __asm__ __volatile__ ("hlt");
    }
}