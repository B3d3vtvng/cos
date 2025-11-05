#include "idt.h"

extern void* isr_stub_table[];

void idt_set_gate(struct idt_entry* idt_entries, int n, uint64_t handler, uint16_t selector, uint8_t flags){
    idt_entries[n].offset_low = handler & 0xFFFF;
    idt_entries[n].selector = selector;
    idt_entries[n].ist = 0;
    idt_entries[n].type_attr = flags;
    idt_entries[n].offset_mid = (handler >> 16) & 0xFFFF;
    idt_entries[n].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt_entries[n].zero = 0;
}

void idt_init(void){
    void* idt_entries = kpmmalloc(sizeof(struct idt_entry) * 256);
    // Set up IDT entries for CPU exceptions (0-31)
    for (int i = 0; i < 32; i++) {
        idt_set_gate(idt_entries, i, (uint64_t)isr_stub_table[i], 0x20, 0x8E);
    }

    // Load the IDT
    struct idt_ptr* idt_pointer = kpmmalloc(sizeof(struct idt_ptr));
    /* Use the actual size of the idt_entries array instead of an undefined macro */
    idt_pointer->limit = (uint16_t)(sizeof(idt_entries) - 1);
    idt_pointer->base = (uint64_t)&idt_entries;

    __asm__ volatile ("lidt %0" : : "m"(idt_pointer));
}

__attribute__((noreturn)) void kernel_panic(const char* message) {
    // Disable interrupts
    __asm__ volatile ("cli");

    vga_clear();
    vga_print("KERNEL PANIC:\n");
    vga_print(message);
    vga_print("\nSystem Halted.");

    // Halt the CPU
    while (1) {
        __asm__ volatile ("hlt");
    }
}