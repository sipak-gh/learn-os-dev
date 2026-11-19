; boot.asm — sprout's boot sector.
;
; The BIOS reads the first 512 bytes of the boot disk to physical address
; 0x7C00 and jumps there in 16-bit real mode, with the boot drive number in
; DL. We set up segments and a stack, then greet the world twice: on the
; screen through the BIOS (int 0x10) and on the first serial port (COM1),
; which `make test` captures. Then we halt.

bits 16
org 0x7C00

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
    mov sp, 0x7C00              ; stack grows down, below our code
    sti
    cld                         ; lodsb walks forwards

    mov [boot_drive], dl        ; the BIOS's only argument to us

    call serial_init

    mov si, msg_hello
    call print
    mov si, msg_drive
    call print
    mov al, [boot_drive]
    call print_hex8
    mov si, msg_crlf
    call print
    mov si, msg_halt
    call print

halt:
    cli
    hlt                         ; sleep until an interrupt; with IF=0
    jmp halt                    ; only an NMI wakes us, so loop

; ----------------------------------------------------------------------------
; serial_init — 38400 baud, 8 data bits, no parity, 1 stop bit (8N1).
;   Clobbers: ax, dx
;   Notes:    The divisor latch (DLAB, bit 7 of the line control register)
;             turns ports +0/+1 into the 16-bit baud rate divisor:
;             115200 / 3 = 38400 baud. QEMU ignores the speed, real UARTs
;             do not.
; ----------------------------------------------------------------------------
serial_init:
    mov dx, COM1 + 1            ; interrupt enable register
    xor al, al
    out dx, al                  ; no UART interrupts: we poll
    mov dx, COM1 + 3            ; line control register
    mov al, 0x80
    out dx, al                  ; DLAB = 1
    mov dx, COM1 + 0
    mov al, 3
    out dx, al                  ; divisor low byte
    mov dx, COM1 + 1
    xor al, al
    out dx, al                  ; divisor high byte
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
    xor bh, bh                  ; page 0 (BL is the colour, ignored in text mode)
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

msg_hello:  db "sprout: hello from the boot sector", 13, 10, 0
msg_drive:  db "sprout: BIOS boot drive ", 0
msg_crlf:   db 13, 10, 0
msg_halt:   db "sprout: halting", 13, 10, 0

boot_drive: db 0

    ; Pad to 510 bytes, then the two signature bytes the BIOS checks.
    times 510 - ($ - $$) db 0
    dw 0xAA55
