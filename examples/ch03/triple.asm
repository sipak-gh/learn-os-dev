; triple.asm — a protected-mode mistake and what it looks like: a triple
; fault. We load DS with selector 0x18, but our GDT has only 3 entries
; (0x00, 0x08, 0x10). The CPU raises a general protection fault (#GP,
; vector 13); there is no IDT to handle it, which is a double fault
; (#DF, vector 8); that can't be handled either: a triple fault, and the
; CPU resets. Run with:  make serial NAME=triple QEMU_D="-d int,cpu_reset"
bits 16
org 0x7C00

start:
    jmp 0:.go
.go:
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov sp, 0x7C00
    mov si, msg
    call puts

    cli
    lgdt [gdt_ptr]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pm

bits 32
pm:
    mov ax, 0x18                ; BUG: one past the end of the GDT
    mov ds, ax                  ; -> #GP -> #DF -> triple fault
    hlt

bits 16
%include "com1.inc"

msg: db "switching to protected mode with a bad selector...", 13, 10, 0

    align 8
gdt:
    dq 0
    dq 0x00CF9A000000FFFF       ; 0x08: code, base 0, limit 4 GiB
    dq 0x00CF92000000FFFF       ; 0x10: data
gdt_ptr:
    dw $ - gdt - 1
    dd gdt

    times 510 - ($ - $$) db 0
    dw 0xAA55
