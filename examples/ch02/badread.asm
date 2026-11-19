; badread.asm — ask int 0x13 AH=0x42 for a sector past the end of the disk
; and print the status the BIOS returns in AH.
bits 16
org 0x7C00

start:
    jmp 0:.go
.go:
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov sp, 0x7C00

    mov ah, 0x42
    mov si, dap                 ; DL is still the boot drive
    int 0x13
    jc .error
    mov si, msg_ok
    call puts
    jmp halt
.error:
    mov si, msg_err
    call puts
    mov al, ah
    call puthex8
    mov si, msg_crlf
    call puts
halt:
    cli
    hlt
    jmp halt

%include "com1.inc"

dap:
    db 16, 0
    dw 1                        ; one sector
    dw 0x8000, 0                ; to 0000:8000
    dq 100000                   ; LBA 100000: ~49 MiB into a 1 MiB disk

msg_ok:   db "read OK?!", 13, 10, 0
msg_err:  db "int 0x13 AH=0x42 failed, AH=0x", 0
msg_crlf: db 13, 10, 0

    times 510 - ($ - $$) db 0
    dw 0xAA55
