[bits 32]

section .text
global switch_stack_and_jmp
extern kmain

; Switch to new stack and jump to kmain
; new_stack: pointer to the top of the new stack
; This function does not return
switch_stack_and_jmp:
    mov esp, [esp + 4]   ; Load new stack pointer from argument
    xor ebp, ebp         ; Clear base pointer
    jmp kmain            ; Jump to kernel main function

    ; Prevent fallthrough
    hlt
    jmp $