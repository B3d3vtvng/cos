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

    ; Call the timer interrupt handler in c, passing the current stack frame as an argument
    mov rdi, rsp
    call handle_timer

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
    