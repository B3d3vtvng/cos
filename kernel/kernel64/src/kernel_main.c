#include "../../kernel.h"

void __attribute__((noreturn)) kmain(struct kern_data* kernel_data){
    vga_print("Reached Kernel main, initializing idt");

    struct idt_ptr idtp = idt_init();
    while (1){
        __asm__ __volatile__ ("hlt");
    }
}