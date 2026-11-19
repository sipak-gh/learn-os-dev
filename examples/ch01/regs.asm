; regs.asm — what state does the BIOS hand us? Print it on COM1, untouched.
;
; We save every register before changing any, then print them in hex.
; IP comes from a call/pop trick: call pushes the address after itself.
bits 16
org 0x7C00

COM1 equ 0x3F8

start:
    ; DS could be anything, so address our variables through CS. That
    ; assumes CS = 0 (the line we print for CS checks it).
    mov [cs:save_ax], ax
    mov [cs:save_ds], ds
    mov [cs:save_sp], sp
    mov [cs:save_ss], ss
    mov [cs:save_dx], dx
    mov [cs:save_cs], cs
    call .here
.here:
    pop ax                      ; ax = address of .here
    sub ax, .here - start       ; = address of start, as the CPU sees it
    mov [cs:save_ip], ax
    pushf
    pop ax
    mov [cs:save_fl], ax

    xor ax, ax
    mov ds, ax
    mov si, names
    mov di, save_cs
    mov cx, 8
.next:
    call puts                   ; "AX=" etc.; si moves to the next name
    mov ax, [di]
    call hex16
    mov al, ' '
    call putc
    add di, 2
    loop .next
    mov al, 13
    call putc
    mov al, 10
    call putc
    cli
    hlt

; putc: al -> COM1 (poll until the transmitter is empty). Keeps all registers.
putc:
    push dx
    push ax
    mov dx, COM1 + 5
.wait:
    in al, dx
    test al, 0x20
    jz .wait
    pop ax
    mov dx, COM1
    out dx, al
    pop dx
    ret

; puts: zero-terminated string at si -> COM1; si ends after the zero.
puts:
    lodsb
    test al, al
    jz .done
    call putc
    jmp puts
.done:
    ret

; hex16: ax as four hex digits. Keeps cx (the caller's loop counter).
hex16:
    push cx
    mov cx, 4
    mov dx, ax
.digit:
    rol dx, 4                   ; top nibble to the bottom
    mov al, dl
    and al, 0x0F
    add al, '0'
    cmp al, '9'
    jbe .out
    add al, 'A' - '9' - 1
.out:
    call putc
    loop .digit
    pop cx
    ret

names:  db "CS=", 0, "IP=", 0, "DS=", 0, "SS=", 0, "SP=", 0, "DX=", 0, "AX=", 0, "FLAGS=", 0
save_cs: dw 0
save_ip: dw 0
save_ds: dw 0
save_ss: dw 0
save_sp: dw 0
save_dx: dw 0
save_ax: dw 0
save_fl: dw 0

    times 510 - ($ - $$) db 0
    dw 0xAA55
