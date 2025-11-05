#ifndef IDT_H
#define IDT_H

#include <stdint.h>
#include "../kernel.h"

struct idt_entry {
    uint16_t offset_low;    // Bits 0-15 of handler function address
    uint16_t selector;      // Kernel segment selector
    uint8_t  ist;           // Bits 0-2 hold Interrupt Stack Table offset, rest of bits zero
    uint8_t  type_attr;     // Type and attributes
    uint16_t offset_mid;    // Bits 16-31 of handler function address
    uint32_t offset_high;   // Bits 32-63 of handler function address
    uint32_t zero;          // Reserved
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

void idt_init(void);

#endif