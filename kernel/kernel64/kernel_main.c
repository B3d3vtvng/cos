#include "include/kernel.h"

void __attribute__((noreturn)) kmain(struct kern_data* kernel_data){
    vga_print("Reached Kernel main");
    while (1){
        __asm__ __volatile__ ("hlt");
    }
}