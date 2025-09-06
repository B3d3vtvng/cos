#include "../io/vga_control.h"
#include "../mem/alloc_init.h"
#include <stdint.h>
#include "../io/conversion.h"

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

struct idt_ptr idt_init(void);