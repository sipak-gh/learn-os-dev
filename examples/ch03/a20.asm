; a20.asm — watch the A20 gate work: close it, see memory wrap at 1 MiB,
; open it again. Uses the "fast A20" bit 1 of system control port 0x92.
bits 16
org 0x7C00

start:
    jmp 0:.go
.go:
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov sp, 0x7C00

    call report                 ; as the BIOS left it

    in al, 0x92
    and al, ~2 & 0xFE           ; clear bit 1 (A20); never set bit 0 (reset)
    out 0x92, al
    mov si, msg_closed
    call puts
    call report

    in al, 0x92
    or al, 2
    and al, 0xFE
    out 0x92, al
    mov si, msg_opened
    call puts
    call report

    cli
    hlt

; report: write 0x00 to 0x000500 and 0xFF to 0x100500 (FFFF:0510), then
; print what 0x000500 holds. 0xFF means both addresses are the same byte.
report:
    push es
    mov ax, 0xFFFF
    mov es, ax
    mov byte [0x0500], 0x00
    mov byte [es:0x0510], 0xFF
    mov si, msg_read
    call puts
    mov al, [0x0500]
    call puthex8
    cmp byte [0x0500], 0xFF
    mov si, msg_wraps
    je .print
    mov si, msg_ok
.print:
    call puts
    pop es
    ret

%include "com1.inc"

msg_closed: db "closed the A20 gate (port 0x92 bit 1 = 0)", 13, 10, 0
msg_opened: db "opened the A20 gate (port 0x92 bit 1 = 1)", 13, 10, 0
msg_read:   db "  byte at 0x000500 after writing 0xFF to 0x100500: 0x", 0
msg_wraps:  db " -> addresses wrap at 1 MiB", 13, 10, 0
msg_ok:     db " -> A20 enabled", 13, 10, 0

    times 510 - ($ - $$) db 0
    dw 0xAA55
