; print16.inc — text output for 16-bit real-mode code: screen and COM1.
;
; Included (assembled into) stage2.asm. Every character goes to the screen
; through the BIOS (int 0x10 teletype) and to the first serial port, which
; `make test` captures. All functions keep every register they don't
; return in, so callers can print anywhere without saving anything.

%ifndef PRINT16_INC
%define PRINT16_INC

COM1        equ 0x3F8
COM1_LSR    equ COM1 + 5
LSR_THRE    equ 0x20

; ----------------------------------------------------------------------------
; putc — one character.
;   In:       al = character
; ----------------------------------------------------------------------------
putc:
    pusha
    mov bl, al
    mov ah, 0x0E                ; BIOS teletype
    xor bh, bh
    int 0x10
    mov dx, COM1_LSR
.wait:
    in al, dx
    test al, LSR_THRE
    jz .wait
    mov dx, COM1
    mov al, bl
    out dx, al
    popa
    ret

; ----------------------------------------------------------------------------
; print — a zero-terminated string.
;   In:       si = string
; ----------------------------------------------------------------------------
print:
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

; PRINT "text" — print a string literal kept next to the code.
; Fine for messages printed once; the string lands in the middle of the
; code, so we jump over it.
%macro PRINT 1
    push si
    jmp %%skip
%%str: db %1, 0
%%skip:
    mov si, %%str
    call print
    pop si
%endmacro

; ----------------------------------------------------------------------------
; print_crlf — end the line ("\r\n": the BIOS and terminals want both).
; ----------------------------------------------------------------------------
print_crlf:
    push ax
    mov al, 13
    call putc
    mov al, 10
    call putc
    pop ax
    ret

; ----------------------------------------------------------------------------
; print_hex32 — eax as "0x" and eight hex digits.
;   In:       eax = value
;   Notes:    32-bit registers work in real mode; the assembler adds an
;             operand-size prefix (0x66) to each such instruction.
; ----------------------------------------------------------------------------
print_hex32:
    push ax
    mov al, '0'
    call putc
    mov al, 'x'
    call putc
    pop ax
    ; fall through
print_hex32_digits:             ; the eight digits, no "0x"
    push eax
    push cx
    push edx
    mov edx, eax
    mov cx, 8
.digit:
    rol edx, 4                  ; the top nibble becomes the bottom one
    mov al, dl
    and al, 0x0F
    add al, '0'
    cmp al, '9'
    jbe .out
    add al, 'a' - '9' - 1
.out:
    call putc
    loop .digit
    pop edx
    pop cx
    pop eax
    ret

; ----------------------------------------------------------------------------
; print_hex64 — the 64-bit value at [si] as "0x" and sixteen hex digits.
;   In:       si = address of a little-endian qword
; ----------------------------------------------------------------------------
print_hex64:
    push eax
    mov eax, [si + 4]           ; high half first: it holds the leading digits
    call print_hex32
    mov eax, [si]
    call print_hex32_digits
    pop eax
    ret

; ----------------------------------------------------------------------------
; print_dec32 — eax as unsigned decimal.
;   In:       eax = value
;   Notes:    Division yields the digits last-first, so we push them on the
;             stack and pop them back in printing order.
; ----------------------------------------------------------------------------
print_dec32:
    push eax
    push ecx
    push edx
    push bx
    mov ecx, 10
    xor bx, bx                  ; digit count
.divide:
    xor edx, edx
    div ecx                     ; eax = v / 10, edx = v % 10
    push dx
    inc bx
    test eax, eax
    jnz .divide
.emit:
    pop ax
    add al, '0'
    call putc
    dec bx
    jnz .emit
    pop bx
    pop edx
    pop ecx
    pop eax
    ret

%endif
