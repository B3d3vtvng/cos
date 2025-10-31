[bits 32]

section .text
    global enter_long_mode
    extern gdt64_ptr
; -------- Switch to 64-bit long mode --------
enter_long_mode:
    sub esp, 8 ; Make space for pt
    mov [esp], edi ; Save pt

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
    mov eax, [esp]
    mov cr3, eax
    add esp, 8 ; Restore rsp

    ; Set cr0.PG
    mov eax, cr0
    or  eax, 1 << 31              ; set PG bit
    mov cr0, eax

    ; Far jump to flush prefetch queue and load CS = 64-bit code selector (0x20)
    jmp 0x20:0x19000      ; jump to 64-bit code segment