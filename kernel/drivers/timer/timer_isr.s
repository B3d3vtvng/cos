[bits 64]

section .text

extern handle_timer

global isr_timer
isr_timer:
    ; disable interrupts so we dont get disrupted
    cli

    ; Push all regs
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; Align stack for the SysV AMD64 ABI (RSP % 16 == 8 before CALL)
    mov rdi, rsp
    sub rsp, 8
    call handle_timer
    add rsp, 8

     ; Restore general purpose registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    ; Return from interrupt
    iretq
    