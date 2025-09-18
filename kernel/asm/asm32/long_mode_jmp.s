[bits 32]

section .text

    global long_mode_jmp
    extern kmain
    extern kernel_dat

long_mode_jmp:
    push kernel_dat
    push 0x00000000
    jmp 0x08:KMAIN_ADDR