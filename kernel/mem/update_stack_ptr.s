[bits 64]

section .text
    global update_stack_ptr

; Updates the stack pointer to the new virtual stack location
; Expects rdi to hold the new stack pointer address
update_stack_ptr:
    mov rsp, rdi
    sub rsp, 8 ; Make space for the return address which is not included in the virtual stack pointer given in rdi
    mov rbp, rsi ; Update base pointer as well
    ret