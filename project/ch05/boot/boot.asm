; boot.asm — sprout's boot sector.
;
; The BIOS reads the first 512 bytes of the boot disk to physical address
; 0x7C00 and jumps there in 16-bit real mode, with the boot drive number in
; DL. 512 bytes is too small for a real loader, so this sector does one job:
; read stage 2 (STAGE2_SECTORS sectors from LBA STAGE2_LBA) to STAGE2_ADDR
; with the BIOS disk services (int 0x13), and jump to it.

bits 16
org 0x7C00

%include "layout.inc"

COM1        equ 0x3F8           ; I/O port base of the first serial port
COM1_LSR    equ COM1 + 5        ; line status register
LSR_THRE    equ 0x20            ; bit 5: transmit holding register empty

start:
    ; The BIOS promises CS:IP points at our first byte, but not which
    ; CS:IP: 0000:7C00 and 07C0:0000 are the same address. A far jump
    ; makes it 0000:7C00, which is what `org 0x7C00` assumed.
    jmp 0x0000:.flush
.flush:
    cli                         ; no interrupts while the stack moves
    xor ax, ax
    mov ds, ax                  ; data at 0000:xxxx, like org assumed
    mov es, ax
    mov ss, ax
    mov sp, BOOT_ADDR           ; stack grows down, below our code
    sti
    cld                         ; lodsb walks forwards

    mov [boot_drive], dl        ; the BIOS's only argument to us

    call serial_init
    mov si, msg_hello
    call print

    ; Does this BIOS have the "extended" disk services (LBA addressing)?
    ; int 0x13, AH=0x41, BX=0x55AA: on success CF=0, BX=0xAA55 and bit 0
    ; of CX says the packet-based functions (AH=0x42 read) work.
    mov ah, 0x41
    mov bx, 0x55AA
    mov dl, [boot_drive]
    int 0x13
    jc no_lba
    cmp bx, 0xAA55
    jne no_lba
    test cx, 1
    jz no_lba

    ; int 0x13, AH=0x42: read the sectors described by the disk address
    ; packet at DS:SI. On error CF=1 and AH holds a status code.
    mov ah, 0x42
    mov dl, [boot_drive]
    mov si, dap
    int 0x13
    jc disk_error

    mov si, msg_loaded
    call print
    mov dl, [boot_drive]        ; stage 2 gets the drive number, too
    jmp 0x0000:STAGE2_ADDR

no_lba:
    mov si, msg_no_lba
    call print
    jmp halt

disk_error:
    mov si, msg_disk_error
    call print
    mov al, ah                  ; the BIOS status code (print kept AH)
    call print_hex8
    mov si, msg_crlf
    call print

halt:
    cli
    hlt                         ; sleep until an interrupt; with IF=0
    jmp halt                    ; only an NMI wakes us, so loop

; ----------------------------------------------------------------------------
; serial_init — 38400 baud, 8 data bits, no parity, 1 stop bit (8N1).
;   Clobbers: ax, dx
; ----------------------------------------------------------------------------
serial_init:
    mov dx, COM1 + 1            ; interrupt enable register
    xor al, al
    out dx, al                  ; no UART interrupts: we poll
    mov dx, COM1 + 3            ; line control register
    mov al, 0x80
    out dx, al                  ; DLAB = 1: ports +0/+1 are the divisor
    mov dx, COM1 + 0
    mov al, 3
    out dx, al                  ; divisor 3: 115200 / 3 = 38400 baud
    mov dx, COM1 + 1
    xor al, al
    out dx, al
    mov dx, COM1 + 3
    mov al, 0x03
    out dx, al                  ; DLAB = 0, 8N1
    ret

; ----------------------------------------------------------------------------
; putc — one character to the screen (BIOS teletype) and to COM1.
;   In:       al = character
;   Clobbers: nothing
; ----------------------------------------------------------------------------
putc:
    pusha
    mov bl, al                  ; keep the character
    mov ah, 0x0E                ; int 0x10, AH=0x0E: teletype output
    xor bh, bh                  ; page 0
    int 0x10

    mov dx, COM1_LSR
.wait:
    in al, dx
    test al, LSR_THRE           ; room for another byte?
    jz .wait
    mov dx, COM1
    mov al, bl
    out dx, al
    popa
    ret

; ----------------------------------------------------------------------------
; print — a zero-terminated string.
;   In:       si = string
;   Clobbers: si, al
; ----------------------------------------------------------------------------
print:
    lodsb                       ; al = [ds:si], si += 1
    test al, al
    jz .done
    call putc
    jmp print
.done:
    ret

; ----------------------------------------------------------------------------
; print_hex8 — a byte as "0x" and two hex digits.
;   In:       al = value
;   Clobbers: ax, cx
; ----------------------------------------------------------------------------
print_hex8:
    mov ah, al                  ; keep the value
    mov al, '0'
    call putc
    mov al, 'x'
    call putc
    mov cl, 4
    mov al, ah
    shr al, cl                  ; high nibble first
    call .digit
    mov al, ah
    and al, 0x0F
.digit:                         ; called for the high nibble, falls in for the low
    cmp al, 10
    jb .num
    add al, 'a' - '0' - 10
.num:
    add al, '0'
    jmp putc                    ; tail call: putc's ret returns for us

; The disk address packet (DAP) for int 0x13 AH=0x42.
dap:
    db 16                       ; size of this packet
    db 0                        ; reserved
    dw STAGE2_SECTORS           ; sectors to read
    dw STAGE2_ADDR, 0           ; buffer, as offset then segment
    dq STAGE2_LBA               ; first sector (LBA, 64 bits)

msg_hello:      db "sprout: boot sector, loading stage 2", 13, 10, 0
msg_loaded:     db "sprout: stage 2 loaded, jumping to it", 13, 10, 0
msg_no_lba:     db "sprout: no LBA disk services, giving up", 13, 10, 0
msg_disk_error: db "sprout: disk read error, status ", 0
msg_crlf:       db 13, 10, 0

boot_drive: db 0

    ; Pad to 510 bytes, then the two signature bytes the BIOS checks.
    times 510 - ($ - $$) db 0
    dw 0xAA55
