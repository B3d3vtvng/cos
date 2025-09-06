#include "../io/vga_control.h"
#include "../mem/alloc_init.h"

extern void __attribute__((noreturn)) switch_stack_and_jmp(void* new_stack);

#define KERN_ENTRY __attribute__((section(".text.entry"))) __attribute__((noreturn))

KERN_ENTRY void _start(void) {
    const char* msg = "Kernel loaded, initializing allocator for stack relocation...\n";

    vga_clear();
    vga_print(msg);
    void* new_kern_stack = init_allocator();
    if (new_kern_stack != (void*)0) {
        // Switch to new stack
        vga_print("Switching to new stack...\n");
        switch_stack_and_jmp(new_kern_stack);
    }

    vga_print("Initialization failed, halting...\n");
    
    // Halt forever (will never be reached until something goes horribly wrong...)
    while (1) {
        __asm__ __volatile__ ("hlt");
    }
}