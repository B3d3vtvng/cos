[bits 32]

%define PML4_ADDR 0x90000

section .text
    global enter_long_mode
    extern gdt64_ptr

; -------- Switch to 64-bit long mode --------
enter_long_mode:
    ; Load gdt
    lgdt [gdt64_ptr]

    ; Set cr4.PAE
    mov eax, cr4
    or  eax, 1 << 5               ; set PAE bit
    mov cr4, eax

    ; Set IA32_EFER.LME
    mov ecx, 0xC0000080           ; IA32_EFER MS
    rdmsr
    or  eax, 1 << 8               ; set LME bit
    wrmsr

    ; Load page table address into cr3
    mov eax, PML4_ADDR
    mov cr3, eax

    mov esp, 0x19000 ; Set up stack for long mode
    mov ebp, 0x19000 ; Set up base pointer for long mode

    ; Load data/stack segments = data selector (0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    ; Set cr0.PG
    mov eax, cr0
    or  eax, 1 << 31              ; set PG bit
    mov cr0, eax

    ; Far jump to flush prefetch queue and load CS = 64-bit code selector (0x08)
    jmp 0x08:0x20000      ; jump to 64-bit code segment