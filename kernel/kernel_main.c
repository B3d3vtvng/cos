#include <stdint.h>

__attribute__((noreturn)) __attribute__((section(".text.kern_entry"))) void kernel_main(void) {
    volatile uint16_t* vga = (uint16_t*)0xB8000;
    const char* msg = "Hello from kernel main!";
    
    for (int i = 0; msg[i] != '\0'; i++) {
        vga[i] = (0x07 << 8) | msg[i]; // Attribute in high byte, char in low byte
    }

    while (1) {
        __asm__ volatile ("hlt");
    }
}