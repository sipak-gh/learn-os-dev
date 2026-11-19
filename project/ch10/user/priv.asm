; priv.asm — a program that tries a privileged instruction (Chapter 9).
;
; cli would stop the timer interrupt and with it preemption: one program
; could keep the CPU forever. In ring 3 with IOPL 0, cli (like hlt, lgdt,
; mov to cr3, ..., and in or out to any port, since sprout's TSS has no
; I/O permission bitmap) raises a general protection fault instead.

bits 32

SYS_WRITE   equ 4
STDOUT      equ 1

section .rodata
msg:    db "priv: trying cli", 10
len     equ $ - msg

section .text
global _start
_start:
    mov eax, SYS_WRITE
    mov ebx, STDOUT
    mov ecx, msg
    mov edx, len
    int 0x80

    cli                         ; #GP: the kernel kills us here
    jmp $
