; Expected setup by Stage 1:
;   - Stage 2 is read to physical 0x1000
;   - Jumped via   jmp 0x0000:0x1000
; Therefore CS = 0x0000, IP = 0x1000, and physical addr = 0x1000.
;

%ifndef __ELF__
    org 0x1000        ; Only valid for flat binary
%endif

bits 16

; Second stage bootloader main function
; Loads the third stage and the kernel and loads the bios memory map
; Also transitions to protected mode, setting up a minimal temporary gdt
start:
    ; Make DS/ES match our code segment base (here CS expected to be 0x0000)
    push cs
    pop ds
    mov ax, ds
    mov es, ax

    ; -------- Real-mode message --------
    mov si, msg
.putc:
    lodsb                   ; AL = [SI], SI++
    test al, al
    jz   load_stage3
    mov ah, 0x0E            ; teletype output
    mov bh, 0x00            ; page
    mov bl, 0x07            ; light gray on black
    int 0x10
    jmp .putc

load_stage3:
    ; Destination buffer = physical STAGE3_BASE
    ; Use ES:BX = 0x1500:0000
    mov ax, 0x1500
    mov es, ax
    mov bx, 0x0000         ; ES:BX = 0x1500:0x0000 -> 0x15000

    mov ah, 0x02                 ; BIOS disk read
    mov al, STAGE3_SEC_CNT         ; number of sectors to read
    mov ch, 0x00                 ; cylinder 0
    mov dh, 0x00                 ; head 0
    mov dl, 0x80            ; drive number from boot
    mov cl, (2 + ST2_SEC_CNT)    ; starting sector (1-based)

    int 0x13
    jc  disk_fail                ; if CF set -> error

    jmp load_kernel          

disk_fail:
    mov si, disk_fail_msg
.df_putc:
    lodsb
    test al, al
    jz   .halt
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x07
    int 0x10
    jmp .df_putc
.halt:
    cli
    hlt
    jmp .halt

; Loads the kernel to 0x20000
load_kernel:
    mov ax, 0x2000
    mov es, ax
    mov bx, 0x0000       ; ES:BX = 0x2000:0x0000 -> 0x2000 * 16 + 0 = 0x20000

    mov ah, 0x02         ; BIOS: read sectors
    mov al, KERN_SEC_CNT ; number of sectors
    mov ch, 0           ; cylinder 0
    mov dh, 0           ; head 0
    mov cl, (2 + ST2_SEC_CNT + STAGE3_SEC_CNT) ; sector
    mov dl, 0x80
    int 0x13

    jc disk_fail       ; jump if error
    jmp load_bios_mmap

load_bios_mmap:
    ; Get BIOS memory map via int 0x15, eax=0xE820
    ; Store entries starting at 0x7504, max 20 bytes each
    ; Store number of entries at 0x7500
    ; We need the bios mmap for the physical memory manager later
    mov ax, 0
    mov es, ax
    mov di, 0x7504              ; buffer starts here
    xor ebx, ebx                 ; continuation = 0
    mov edx, 0x534D4150          ; 'SMAP'
    mov ecx, 20                  ; entry size we want
    mov esi, 0                   ; entry counter

.next_entry:
    mov eax, 0xE820
    int 0x15
    jc .done
    cmp eax, 0x534D4150
    jne .done

    inc esi                      ; count entry
    add edi, 20                  ; advance buffer pointer

    test ebx, ebx
    jnz .next_entry

.done:
    mov [0x7500], esi           ; store entry count at 0x7500
    jmp enter_prot_mode


; Switch to 32-bit protected mode
enter_prot_mode:
    cli
    lgdt [gdt_descriptor]        ; load our flat GDT

    mov eax, cr0
    or  eax, 1                   ; set PE bit
    mov cr0, eax

    ; Set up stack for protected mode
    mov esp, 0x14FF0
    mov ebp, 0x14FF0

    ; Far jump to flush prefetch queue and load CS = code selector (0x08)
    jmp 0x08:pm_entry          ; jump to 32-bit code segment

; -------- 32-bit code --------
bits 32
pm_entry:
    ; Load data/stack segments = data selector (0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    ; Jump to stage3 entry (flat linear address)
    mov eax, 0x15000
    jmp eax                       ; transfer control to stage 3



; -------- Data --------
[bits 16]
msg             db "Stage 2: Entering 32-bit protected mode...", 0x0D, 0x0A, 0
disk_fail_msg   db "Stage 2: Disk read failed!", 0

; -------- GDT (flat 0..4GiB) --------
; 0x08 = code selector, 0x10 = data selector
gdt_start:
    dq 0x0000000000000000        ; null descriptor
    dq 0x00CF9A000000FFFF        ; code: base=0, limit=4GiB, RX
    dq 0x00CF92000000FFFF        ; data: base=0, limit=4GiB, RW
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start
