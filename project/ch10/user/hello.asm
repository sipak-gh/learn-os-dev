; hello.asm — hello, world from ring 3 (Chapter 9).
;
; Two system calls with 32-bit Linux's numbers and registers: the same
; program is also a valid Linux i386 executable.

bits 32

SYS_EXIT    equ 1
SYS_WRITE   equ 4
STDOUT      equ 1

section .rodata
msg:    db "hello from ring 3", 10
len     equ $ - msg

section .text
global _start
_start:
    mov eax, SYS_WRITE          ; write(STDOUT, msg, len)
    mov ebx, STDOUT
    mov ecx, msg
    mov edx, len
    int 0x80

    mov eax, SYS_EXIT           ; exit(0)
    xor ebx, ebx
    int 0x80
