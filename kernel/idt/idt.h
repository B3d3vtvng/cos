#ifndef IDT_H
#define IDT_H

#include <stdint.h>
#include "../mem/pmmalloc.h"
#include "../util/conversion.h"
#include "../util/string.h"

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

typedef struct excp_frame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags;
    // rsp and ss only if privilege level change (user → kernel)
    uint64_t rsp, ss;
} excp_frame_t;

struct idt_ptr idt_init(void);
void __attribute__((noreturn)) kernel_panic(struct excp_frame* register_state, const char* message);

#endif