; a20.inc — make sure address line 20 works (16-bit real mode code).
;
; The 8086 had 20 address lines; FFFF:0010 wrapped around to 0000:0000 and
; some DOS programs depended on it. The IBM AT (80286, 24 lines) kept the
; wrap by gating line 20 through a spare pin of the keyboard controller:
; with the "A20 gate" closed, bit 20 of every address is forced to 0, so
; 1 MiB + x reads x. We want all memory, so the gate must be open.

%ifndef A20_INC
%define A20_INC

; ----------------------------------------------------------------------------
; a20_check — is the A20 line enabled?
;   Out:      ZF=0 (jnz) if enabled, ZF=1 (jz) if addresses wrap at 1 MiB
;   Clobbers: nothing but flags
;   Notes:    0000:0500 is physical 0x000500 and FFFF:0510 is physical
;             0x100500. Write different bytes to both; if the first one
;             changes when we write the second, they are the same byte.
; ----------------------------------------------------------------------------
a20_check:
    pushf                       ; we need IF back as it was (cli below)
    push ds
    push es
    push di
    push si
    push ax
    cli
    xor ax, ax
    mov es, ax
    mov di, 0x0500              ; es:di = 0x000500
    not ax
    mov ds, ax
    mov si, 0x0510              ; ds:si = 0x100500 (or 0x000500 if wrapping)
    mov al, [es:di]             ; save both bytes
    mov ah, [ds:si]
    push ax
    mov byte [es:di], 0x00
    mov byte [ds:si], 0xFF
    cmp byte [es:di], 0xFF      ; ZF=1: the write went to 0x000500
    pop ax
    mov [ds:si], ah             ; restore (in this order: if they alias,
    mov [es:di], al             ; the low byte's old value wins)
    pop ax                      ; pop and mov keep flags; only popf would
    pop si                      ; change them, so we pass ZF out through
    pop di                      ; the flags image on the stack:
    pop es
    pop ds
    jz .wraps
    popf
    or sp, sp                   ; sp is never 0: ZF=0, "enabled"
    ret
.wraps:
    popf
    cmp sp, sp                  ; ZF=1, "wraps"
    ret

; ----------------------------------------------------------------------------
; a20_enable — open the A20 gate if it is closed, trying the BIOS first
;              and then the "fast A20" bit in system control port 0x92.
;   Out:      CF=0 if A20 is enabled now, CF=1 if every method failed
;   Clobbers: ax
;   Notes:    The third classic method talks to the 8042 keyboard
;             controller (command 0xD1, write output port, bit 1). QEMU,
;             like most machines since the 1990s, has port 0x92, so we
;             leave the 8042 dance to the chapter text.
; ----------------------------------------------------------------------------
a20_enable:
    call a20_check
    jnz .on

    mov ax, 0x2401              ; int 0x15, AX=0x2401: BIOS, enable A20
    int 0x15
    call a20_check
    jnz .on

    in al, 0x92                 ; port 0x92 bit 1: A20 gate (bit 0 would
    test al, 2                  ; reset the machine: never set it here)
    jnz .no_fast
    or al, 2
    and al, 0xFE
    out 0x92, al
.no_fast:
    call a20_check
    jnz .on
    stc
    ret
.on:
    clc
    ret

%endif
