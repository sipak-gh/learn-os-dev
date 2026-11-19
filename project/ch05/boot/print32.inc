; print32.inc — text output for 32-bit protected-mode code.
;
; No BIOS any more: we write characters straight into the VGA text buffer
; at 0xB8000 (80x25 cells of two bytes: character, then colour) and into
; the serial port's transmit register. Strings end lines with "\n" alone;
; pm_putc sends "\r\n" to the serial port for terminals.
;
; Include after print16.inc (it reuses COM1 and LSR_THRE).

%ifndef PRINT32_INC
%define PRINT32_INC

VGA_TEXT    equ 0xB8000
VGA_COLS    equ 80
VGA_ROWS    equ 25
BDA_CURSOR  equ 0x0450          ; BIOS data area: page 0 cursor (column, row)
VGA_CRTC    equ 0x3D4           ; CRT controller: index port, data at +1

bits 32

; ----------------------------------------------------------------------------
; pm_con_init — continue the screen where the BIOS left its cursor.
;   Clobbers: eax
; ----------------------------------------------------------------------------
pm_con_init:
    movzx eax, byte [BDA_CURSOR]
    mov [con_col], eax
    movzx eax, byte [BDA_CURSOR + 1]
    mov [con_row], eax
    ret

; ----------------------------------------------------------------------------
; pm_putc — one character to the screen (colour con_attr) and to COM1.
;   In:       al = character
;   Clobbers: nothing
; ----------------------------------------------------------------------------
pm_putc:
    pushad
    mov bl, al
    cmp bl, 10
    jne .serial
    mov al, 13                  ; "\n" -> "\r\n" on the serial line
    call serial_out
.serial:
    mov al, bl
    call serial_out

    cmp bl, 10
    je .newline
    mov eax, [con_row]
    imul eax, VGA_COLS
    add eax, [con_col]          ; eax = cell index
    mov bh, [con_attr]
    mov [VGA_TEXT + eax * 2], bx ; character in the low byte, colour high
    inc dword [con_col]
    cmp dword [con_col], VGA_COLS
    jb .cursor
.newline:
    mov dword [con_col], 0
    inc dword [con_row]
    cmp dword [con_row], VGA_ROWS
    jb .cursor
    ; Off the bottom: move rows 1-24 up one row, blank row 24.
    mov esi, VGA_TEXT + VGA_COLS * 2
    mov edi, VGA_TEXT
    mov ecx, VGA_COLS * (VGA_ROWS - 1) / 2  ; dwords: two cells each
    rep movsd
    mov eax, 0x07200720         ; two blank cells, grey on black
    mov ecx, VGA_COLS / 2
    rep stosd
    mov dword [con_row], VGA_ROWS - 1
.cursor:
    call vga_move_cursor
    popad
    ret

; serial_out — al to COM1 once the transmitter is empty. Clobbers dx.
serial_out:
    push eax
    mov dx, COM1_LSR
.wait:
    in al, dx
    test al, LSR_THRE
    jz .wait
    pop eax
    mov dx, COM1
    out dx, al
    ret

; vga_move_cursor — put the blinking hardware cursor at con_row/con_col.
; CRTC registers 0x0E/0x0F hold the cursor's cell index, high byte first.
; Clobbers eax, ecx, edx.
vga_move_cursor:
    mov ecx, [con_row]
    imul ecx, VGA_COLS
    add ecx, [con_col]
    mov dx, VGA_CRTC
    mov al, 0x0E
    out dx, al
    inc dx
    mov al, ch
    out dx, al
    dec dx
    mov al, 0x0F
    out dx, al
    inc dx
    mov al, cl
    out dx, al
    ret

; ----------------------------------------------------------------------------
; pm_print — a zero-terminated string.
;   In:       esi = string
;   Clobbers: nothing
; ----------------------------------------------------------------------------
pm_print:
    push esi
    push eax
.next:
    lodsb
    test al, al
    jz .done
    call pm_putc
    jmp .next
.done:
    pop eax
    pop esi
    ret

; PM_PRINT "text" — the 32-bit twin of PRINT.
%macro PM_PRINT 1
    push esi
    jmp %%skip
%%str: db %1, 0
%%skip:
    mov esi, %%str
    call pm_print
    pop esi
%endmacro

; ----------------------------------------------------------------------------
; pm_print_hex — eax as "0x" and eight hex digits.
;   Clobbers: nothing
; ----------------------------------------------------------------------------
pm_print_hex:
    pushad
    mov edx, eax
    mov al, '0'
    call pm_putc
    mov al, 'x'
    call pm_putc
    mov ecx, 8
.digit:
    rol edx, 4
    mov al, dl
    and al, 0x0F
    add al, '0'
    cmp al, '9'
    jbe .out
    add al, 'a' - '9' - 1
.out:
    call pm_putc
    loop .digit
    popad
    ret

con_row:    dd 0
con_col:    dd 0
con_attr:   db 0x0A             ; light green on black

bits 16                         ; back to the default for whoever includes us

%endif
