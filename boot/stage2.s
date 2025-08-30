bits 16
org 0x1000                ; Stage 2 loads here

gdt_start:
    dq 0x0000000000000000 ; Null descriptor
    dq 0x00CF9A000000FFFF ; Code segment descriptor
    dq 0x00CF92000000FFFF ; Data segment descriptor
gdt_end:

gdt_table:
    dw gdt_end - gdt_start - 1 ; Limit
    dd gdt_start               ; Base

start:
    ; Print message using BIOS teletype (int 10h, AH=0Eh)
    mov si, msg
.print_next:
    lodsb                 ; AL = [SI], SI++
    test al, al
    jz .activate_pmode
    mov ah, 0x0E
    mov bx, 0x0007        ; BH=page 0, BL=attribute (gray on black)
    int 0x10
    jmp .print_next

.activate_pmode:
    cli                     ; Clear interrupts
    lgdt [gdt_table]       ; Load GDT
    mov eax, cr0
    or eax, 1              ; Set PE bit
    mov cr0, eax           ; Enter protected mode
    jmp 0x08:.pmode_start  ; Far jump to flush prefetch queue

.pmode_start:
    ; Set up segment registers
    mov ax, 0x10           ; Data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ; Set up stack
    mov esp, 0x9FC00       ; Stack pointer (below 640KB

    ; Now in 32-bit protected mode
    ; Load the Kernel
    call load_kernel

load_kernel:
    ; Load Kernel into KERNEL_BASE
    xor bx, bx
    mov es, KERNEL_BASE

    mov ah, 0x02        ; BIOS: read sectors
    mov al, KERNEL_SEC_CNT ; number of sectors
    mov ch, 0           ; cylinder 0
    mov cl, 2 + ST2_SEC_CNT ; sector (after boot + stage
    mov dh, 0           ; head 0
    mov dl, 0           ; drive (set by BIOS in DL)
    int 0x13

    jc .disk_fail       ; jump if error
    jmp KERNEL_BASE:0x0000 ; jump to Kernel code

.disk_fail:
    mov si, disk_fail_msg
    jmp .print_fail_loop
.print_fail_loop:
    lodsb                 ; AL = [SI], SI++
    test al, al
    jz .hang
    mov ah, 0x0E
    mov bx, 0x0007        ; BH=page 0, BL=
    int 0x10
    jmp .print_fail_loop
.hang:
    hlt
    jmp .hang

msg db "Stage 2 loaded. Switching into 32-bit protected mode...", 0xd, 0xa, 0x0
disk_fail_msg db "Fatal Error: Disk read failed!", 0x0



