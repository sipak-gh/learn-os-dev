; snoop.asm — a program that wants to read the kernel (Chapter 9).
;
; First it asks the kernel to write() kernel memory, and memory that
; isn't mapped at all, to the console; the kernel checks the pointer and
; refuses with -EFAULT. Then it reads the kernel's first bytes itself:
; the page is mapped (the kernel is in every address space) but not
; PAGE_USER, so the CPU raises a page fault and the kernel kills snoop.

bits 32

SYS_EXIT    equ 1
SYS_WRITE   equ 4
STDOUT      equ 1
EFAULT      equ 14
KERNEL      equ 0x100000        ; where the kernel's code starts
NOWHERE     equ 0x50000000      ; user range, but nothing mapped there

; WRITE addr, len — write(STDOUT, addr, len); the result is in eax.
%macro WRITE 2
    mov eax, SYS_WRITE
    mov ebx, STDOUT
    mov ecx, %1
    mov edx, %2
    int 0x80
%endmacro

; SAY "text" — write a line.
%macro SAY 1
[section .rodata]
%%s:    db %1, 10
%%len   equ $ - %%s
__SECT__
    WRITE %%s, %%len
%endmacro

section .text
global _start
_start:
    WRITE NOWHERE, 16
    cmp eax, -EFAULT
    jne .exit
    SAY "snoop: write(1, 0x50000000, 16) = -EFAULT"

    WRITE KERNEL, 16
    cmp eax, -EFAULT
    jne .exit
    SAY "snoop: write(1, 0x00100000, 16) = -EFAULT"

    SAY "snoop: reading 0x00100000 myself"
    mov eax, [KERNEL]           ; page fault: the kernel kills us here
    SAY "snoop: got it!"        ; not reached

.exit:
    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80
