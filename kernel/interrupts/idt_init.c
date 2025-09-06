#include "idt.h"

#define HANDLER_DECLARE(n) extern void isr_##n();
#define HANDLER_ADD(n) set_idt_gate(idt, n, (uint32_t)isr_##n);

HANDLER_DECLARE(0)
HANDLER_DECLARE(1)
HANDLER_DECLARE(2)
HANDLER_DECLARE(3)
HANDLER_DECLARE(4)
HANDLER_DECLARE(5)
HANDLER_DECLARE(6)
HANDLER_DECLARE(7)
HANDLER_DECLARE(8)
HANDLER_DECLARE(9)
HANDLER_DECLARE(10)
HANDLER_DECLARE(11)
HANDLER_DECLARE(12)
HANDLER_DECLARE(13)
HANDLER_DECLARE(14)
HANDLER_DECLARE(15)
HANDLER_DECLARE(16)
HANDLER_DECLARE(17)
HANDLER_DECLARE(18)
HANDLER_DECLARE(19)
HANDLER_DECLARE(20)
HANDLER_DECLARE(21)
HANDLER_DECLARE(22)
HANDLER_DECLARE(23)
HANDLER_DECLARE(24)
HANDLER_DECLARE(25)
HANDLER_DECLARE(26)
HANDLER_DECLARE(27)
HANDLER_DECLARE(28)
HANDLER_DECLARE(29)
HANDLER_DECLARE(30)
HANDLER_DECLARE(31)
HANDLER_DECLARE(unknown)

struct idt_entry {
    uint16_t offset_low;    // Lower 16 bits of handler function address
    uint16_t selector;      // Kernel segment selector
    uint8_t  zero;          // This must always be zero
    uint8_t  type_attr;     // Type and attributes
    uint16_t offset_high;   // Upper 16 bits of handler function address
} __attribute__((packed)); // Full size: 8 bytes

struct idt_ptr {
    uint16_t limit; // Size of the IDT - 1
    uint32_t base;  // Base address of the first element in the IDT
} __attribute__((packed)); // Full size: 6 bytes

#define IDT_SIZE 256

void excp_handler_common(const char* excp_str){
    vga_print("Exception occurred: ");
    vga_print(excp_str);
    vga_print("\n\nSystem halted...\n");
    while(1){
        __asm__ __volatile__ ("hlt");
    }
}

void set_idt_gate(struct idt_entry* idt, int n, uint32_t handler_addr) {
    idt[n].offset_low = handler_addr & 0xFFFF;
    idt[n].selector = 0x08;      // kernel code segment in GDT
    idt[n].zero = 0;
    idt[n].type_attr = 0x8E;     // interrupt gate, present
    idt[n].offset_high = (handler_addr >> 16) & 0xFFFF;
}

void idt_install(struct idt_ptr* idt_ptr) {
    __asm__ __volatile__ ("lidt (%0)" : : "r" (idt_ptr));
}


struct idt_ptr idt_init(void){
    struct idt_entry* idt = kpgmalloc(1); // Allocate one page (4096 bytes) for IDT (even though its only 2048 bytes)
    struct idt_ptr idt_pointer;
    idt_pointer.limit = (sizeof(struct idt_entry) * IDT_SIZE) - 1;
    idt_pointer.base = (uint32_t)idt;

    // Set all IDT entries to a default handler
    for (int i = 0; i < IDT_SIZE; i++) {
        set_idt_gate(idt, i, (uint32_t)isr_unknown);
    }

    HANDLER_ADD(0)
    HANDLER_ADD(1)
    HANDLER_ADD(2)
    HANDLER_ADD(3)
    HANDLER_ADD(4)
    HANDLER_ADD(5)
    HANDLER_ADD(6)
    HANDLER_ADD(7)
    HANDLER_ADD(8)
    HANDLER_ADD(9)
    HANDLER_ADD(10)
    HANDLER_ADD(11)
    HANDLER_ADD(12)
    HANDLER_ADD(13)
    HANDLER_ADD(14)
    HANDLER_ADD(15)
    HANDLER_ADD(16)
    HANDLER_ADD(17)
    HANDLER_ADD(18)
    HANDLER_ADD(19)
    HANDLER_ADD(20)
    HANDLER_ADD(21)
    HANDLER_ADD(22)
    HANDLER_ADD(23)
    HANDLER_ADD(24)
    HANDLER_ADD(25)
    HANDLER_ADD(26)
    HANDLER_ADD(27)
    HANDLER_ADD(28)
    HANDLER_ADD(29)
    HANDLER_ADD(30)
    HANDLER_ADD(31)

    idt_install(&idt_pointer);

    vga_print("IDT initialized and loaded.\n");
    return idt_pointer;
}

