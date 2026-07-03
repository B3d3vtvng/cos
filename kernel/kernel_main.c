#include "kernel.h"

struct kernel_metadata kernel_meta;

void init_devices(void){
    init_apic(kernel_meta.idt_ptr);
    //init_fs();
    timer_init(kernel_meta.idt_ptr);
    //init_keyboard();
}

void print_msg(struct proc_state* pstate){
    vga_print("Hello from timer");
}

__attribute__((noreturn)) __attribute__((section(".text.kern_entry"))) void kernel_main(void) {
    //Save the hardcoded pagetable address in our kernel metadata struct
    kernel_meta.pagetable_address = PAGETABLE_ADDRESS;

    //Clear the screen and print a message
    vga_clear();
    vga_print("Welcome to COS Kernel!\n\n");

    // Initialize physical memory manager
    init_pmm();

    /* Create a new gdt to be able to start usermode processes. 
    Also define a tss to be able to handle exceptions like page faults or double faults in a safe way.
    */
    kernel_meta.gdt_ptr = init_gdt_and_tss();
    vga_print("Initialized full gdt and tss\n");

    // Initialize a primitive interrupt descriptor table to manage hardware exceptions (Simply panics in this state)
    kernel_meta.idt_ptr = idt_init();
    vga_print("Initialized idt\n");

    // Initialize virtual memory allocator, remap kernel and neccessary kernel ram to high memory (0xFFFF800000000000), map everything else to low memory (0x0+)
    vmm_init(&kernel_meta.gdt_ptr, &kernel_meta.idt_ptr);

    init_devices();
    //init_syscalls();
    //init_sched();

    //sched_start();

    notify_once(1000, print_msg);

    while (1){
        __asm__ __volatile__("hlt");
    }
}