section .text

    global long_mode_jmp
    extern kmain
    extern kernel_dat

long_mode_jmp:
    mov rdi, kernel_dat
    jmp 0x08:kmain