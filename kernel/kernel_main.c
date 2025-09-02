#include "io/vga_control.h"

void init_subsystems(void);

__attribute__((noreturn)) 
__attribute__((section(".text.entry")))
void kmain(void) {
    const char* msg = "Kernel loaded, initializing neccessary components...";

    vga_clear();
    vga_print(msg);
    init_subsystems();
    
    // Halt forever
    while (1) {
        __asm__ __volatile__ ("hlt");
    }
}



void init_subsystems(void) {

}