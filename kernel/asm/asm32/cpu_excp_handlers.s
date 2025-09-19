section .rodata

excp_str_0: db "Divide Error", 0x0
excp_str_1: db "Debug", 0x0
excp_str_2: db "Non-Maskable Interrupt", 0x0
excp_str_3: db "Breakpoint", 0x0
excp_str_4: db "Overflow", 0x0
excp_str_5: db "Bound Range Exceeded", 0x0
excp_str_6: db "Invalid Opcode", 0x0
excp_str_7: db "Device Not Available", 0x0
excp_str_8: db "Double Fault", 0x0
excp_str_9: db "Coprocessor Segment Overrun", 0x0
excp_str_10: db "Invalid TSS", 0x0
excp_str_11: db "Segment Not Present", 0x0
excp_str_12: db "Stack Segment Fault", 0x0
excp_str_13: db "General Protection Fault", 0x0
excp_str_14: db "Page Fault", 0x0
excp_str_15: db "Reserved", 0x0
excp_str_16: db "x87 Floating-Point Error", 0x0
excp_str_17: db "Alignment Check", 0x0
excp_str_18: db "Machine Check", 0x0
excp_str_19: db "SIMD Floating-Point Exception", 0x0
excp_str_20: db "Virtualization Exception", 0x0
excp_str_21: db "Reserved", 0x0
excp_str_22: db "Reserved", 0x0
excp_str_23: db "Reserved", 0x0
excp_str_24: db "Reserved", 0x0
excp_str_25: db "Reserved", 0x0
excp_str_26: db "Reserved", 0x0
excp_str_27: db "Reserved", 0x0
excp_str_28: db "Reserved", 0x0
excp_str_29: db "Reserved", 0x0
excp_str_30: db "Security Exception", 0x0
excp_str_31: db "Reserved", 0x0
excp_str_unknown: db "Unknown Exception", 0x0

section .text
    extern excp_handler_common


    global isr_0
isr_0:
    push excp_str_0
    call excp_handler_common


    global isr_1
isr_1:
    push excp_str_1
    call excp_handler_common


    global isr_2
isr_2:
    push excp_str_2
    call excp_handler_common


    global isr_3
isr_3:
    push excp_str_3
    call excp_handler_common


    global isr_4
isr_4:
    push excp_str_4
    call excp_handler_common


    global isr_5
isr_5:
    push excp_str_5
    call excp_handler_common


    global isr_6
isr_6:
    push excp_str_6
    call excp_handler_common


    global isr_7
isr_7:
    push excp_str_7
    call excp_handler_common


    global isr_8
isr_8:
    push excp_str_8
    call excp_handler_common


    global isr_9
isr_9:
    push excp_str_9
    call excp_handler_common


    global isr_10
isr_10:
    push excp_str_10
    call excp_handler_common


    global isr_11
isr_11:
    push excp_str_11
    call excp_handler_common


    global isr_12
isr_12:
    push excp_str_12
    call excp_handler_common


    global isr_13
isr_13:
    push excp_str_13
    call excp_handler_common


    global isr_14
isr_14:
    push excp_str_14
    call excp_handler_common


    global isr_15
isr_15:
    push excp_str_15
    call excp_handler_common


    global isr_16
isr_16:
    push excp_str_16
    call excp_handler_common


    global isr_17
isr_17:
    push excp_str_17
    call excp_handler_common


    global isr_18
isr_18:
    push excp_str_18
    call excp_handler_common


    global isr_19
isr_19:
    push excp_str_19
    call excp_handler_common


    global isr_20
isr_20:
    push excp_str_20
    call excp_handler_common


    global isr_21
isr_21:
    push excp_str_21
    call excp_handler_common


    global isr_22
isr_22:
    push excp_str_22
    call excp_handler_common


    global isr_23
isr_23:
    push excp_str_23
    call excp_handler_common


    global isr_24
isr_24:
    push excp_str_24
    call excp_handler_common


    global isr_25
isr_25:
    push excp_str_25
    call excp_handler_common


    global isr_26
isr_26:
    push excp_str_26
    call excp_handler_common


    global isr_27
isr_27:
    push excp_str_27
    call excp_handler_common


    global isr_28
isr_28:
    push excp_str_28
    call excp_handler_common


    global isr_29
isr_29:
    push excp_str_29
    call excp_handler_common


    global isr_30
isr_30:
    push excp_str_30
    call excp_handler_common


    global isr_31
isr_31:
    push excp_str_31
    call excp_handler_common

    global isr_unknown
isr_unknown:
    push excp_str_unknown
    call excp_handler_common