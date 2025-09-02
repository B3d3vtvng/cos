bits 16
%ifndef __ELF__
    org 0x7C00                ; BIOS loads us here
%endif

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00        ; simple stack

    ; Print message using BIOS teletype (int 10h, AH=0Eh)
    mov si, msg
.print_next:
    lodsb                 ; AL = [SI], SI++
    test al, al
    jz .load_st2
    mov ah, 0x0E
    mov bx, 0x0007        ; BH=page 0, BL=attribute (gray on black)
    int 0x10
    jmp .print_next

.load_st2:
    ; Load Stage 2 into 0x1000
    xor bx, bx
    mov es, bx
    mov bx, 0x1000       ; ES:BX = 0x0000:0x1000

    mov ah, 0x02        ; BIOS: read sectors
    mov al, ST2_SEC_CNT ; number of sectors
    mov ch, 0           ; cylinder 0
    mov cl, 2           ; sector 2 (sector 1 = boot sector)
    mov dh, 0           ; head 0
    mov dl, 0           ; drive (set by BIOS in DL)

    int 0x13
    jc .disk_fail       ; jump if error

    jmp 0x0000:0x1000   ; jump to Stage 2 code

.disk_fail:
    mov si, disk_fail_msg
    jmp .print_fail_loop
.print_fail_loop:
    lodsb                 ; AL = [SI], SI++
    test al, al
    jz .hang
    mov ah, 0x0E
    mov bx, 0x0007        ; BH=page 0, BL=attribute (gray on black)
    int 0x10
    jmp .print_fail_loop

.hang:
    hlt
    jmp .hang

msg db "Booting CoolOS...", 0xd, 0xa, 0x0
disk_fail_msg db "Fatal Error: Disk read failed!", 0x0

; --- Pad to 510 bytes and add 0xAA55 signature ---
times 510-($-$$) db 0
dw 0xAA55
