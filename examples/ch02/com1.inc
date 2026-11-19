; com1.inc — minimal COM1 output for the examples (no init: the BIOS
; default works in QEMU). putc keeps all registers.
putc:
    push dx
    push ax
    mov dx, 0x3F8 + 5
.wait:
    in al, dx
    test al, 0x20
    jz .wait
    pop ax
    mov dx, 0x3F8
    out dx, al
    pop dx
    ret

; puts: zero-terminated string at ds:si.
puts:
    push si
    push ax
.next:
    lodsb
    test al, al
    jz .done
    call putc
    jmp .next
.done:
    pop ax
    pop si
    ret

; putd: ax as unsigned decimal.
putd:
    pusha
    mov bx, 10
    xor cx, cx
.div:
    xor dx, dx
    div bx
    push dx
    inc cx
    test ax, ax
    jnz .div
.out:
    pop ax
    add al, '0'
    call putc
    loop .out
    popa
    ret

; puthex8: al as two hex digits.
puthex8:
    push ax
    push ax
    shr al, 4
    call .nib
    pop ax
    call .nib
    pop ax
    ret
.nib:
    and al, 0x0F
    add al, '0'
    cmp al, '9'
    jbe .ok
    add al, 7
.ok:
    jmp putc
