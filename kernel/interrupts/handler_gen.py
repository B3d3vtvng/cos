#!/usr/bin/env python3

exceptions = [
    "Divide Error",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Error",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Security Exception",
    "Reserved"
]

print('''
section .rodata
''')

for i, name in enumerate(exceptions):
    print(f'excp_str_{i}: db "{name}", 0x0')
    
print("excp_str_unknown: \"Unknown Exception\", 0x0")

print('''
section .text
    extern excp_handler_common
''')

for i, name in enumerate(exceptions):
    print(f'''
    global isr_{i}
isr_{i}:
    push excp_str_{i}
    call excp_handler_common
''')
    
print("global isr_unknown")
print("isr_unknown:\npush excp_str_unknown\ncall excp_handler_common")
