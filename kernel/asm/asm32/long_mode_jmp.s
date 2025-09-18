[bits 32]

section .text

    global long_mode_jmp
    extern kmain
    extern kernel_dat
    extern gdt64_ptr

long_mode_jmp:
    lgdt [gdt64_ptr]

    ; Enable long mode in IA32_EFER MSR
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    push kernel_dat
    push 0x00000000
    jmp 0x08:KMAIN_ADDR