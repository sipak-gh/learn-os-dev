; chs.asm — read LBA 1 the old way: cylinder/head/sector (int 0x13 AH=0x02).
;
; First ask the BIOS for the disk geometry (AH=0x08), convert LBA 1 to
; CHS with it, read that sector to 0x8000 and print the text it holds.
bits 16
org 0x7C00

LBA equ 1

start:
    jmp 0:.go
.go:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    mov [drive], dl

    ; AH=0x08: get drive parameters. Returns DH = last head number,
    ; CL bits 0-5 = sectors per track, CH + CL bits 6-7 = last cylinder.
    mov ah, 0x08
    xor di, di                  ; ES:DI = 0:0 works around old BIOS bugs
    int 0x13
    jc fail
    movzx ax, dh
    inc ax
    mov [heads], ax
    mov al, cl
    and ax, 0x3F
    mov [spt], ax
    mov al, ch                  ; low 8 bits of the last cylinder
    mov ah, cl
    shr ah, 6                   ; bits 8-9
    inc ax
    mov [cyls], ax

    mov si, msg_geom
    call puts
    mov ax, [cyls]
    call putd
    mov si, msg_c
    call puts
    mov ax, [heads]
    call putd
    mov si, msg_h
    call puts
    mov ax, [spt]
    call putd
    mov si, msg_s
    call puts

    ; LBA -> CHS:  sector   = LBA % spt + 1   (sectors count from 1!)
    ;              head     = (LBA / spt) % heads
    ;              cylinder = (LBA / spt) / heads
    mov ax, LBA
    xor dx, dx
    div word [spt]              ; ax = LBA / spt, dx = LBA % spt
    inc dx
    mov [sec], dx
    xor dx, dx
    div word [heads]            ; ax = cylinder, dx = head
    mov [cyl], ax
    mov [head], dx

    mov si, msg_lba
    call puts
    mov ax, [cyl]
    call putd
    mov al, '/'
    call putc
    mov ax, [head]
    call putd
    mov al, '/'
    call putc
    mov ax, [sec]
    call putd
    mov si, msg_crlf
    call puts

    ; AH=0x02: read AL sectors. CH = cylinder low 8 bits, CL = sector
    ; (bits 0-5) + cylinder bits 8-9 (bits 6-7), DH = head, ES:BX = buffer.
    mov ax, [cyl]
    mov ch, al
    mov cl, ah
    shl cl, 6
    or cl, [sec]
    mov dh, [head]
    mov dl, [drive]
    mov bx, 0x8000
    mov ax, 0x0201              ; AH=0x02 read, AL=1 sector
    int 0x13
    jc fail
    mov si, 0x8000
    call puts
    jmp halt

fail:
    mov si, msg_fail
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

msg_geom: db "geometry: ", 0
msg_c:    db " cylinders, ", 0
msg_h:    db " heads, ", 0
msg_s:    db " sectors per track", 13, 10, 0
msg_lba:  db "LBA 1 = C/H/S ", 0
msg_fail: db "int 0x13 failed, AH=0x", 0
msg_crlf: db 13, 10, 0

drive: db 0
heads: dw 0
spt:   dw 0
cyls:  dw 0
cyl:   dw 0
head:  dw 0
sec:   dw 0

    times 510 - ($ - $$) db 0
    dw 0xAA55
