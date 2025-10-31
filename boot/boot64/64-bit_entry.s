[bits 64]
[org 0x19000]

section .text
    global start64

; -------- 64-bit entry point --------
start64:
    ; Set up data segments
    mov ax, 0x18
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax
    mov rsp, 0x20000              ; simple stack, may overwrite earlier data but we dont care :)
    mov rbp, 0x20000

    ; Jump to kernel entry (flat linear address)
    jmp 0x20000