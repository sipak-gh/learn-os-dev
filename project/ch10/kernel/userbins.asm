; userbins.asm — the user programs, built from user/, inside the kernel.
;
; sprout has no file system yet, so the programs' ELF files travel inside
; the kernel image: incbin copies a file's bytes into the output.
; user_programs is a table of struct program (kernel/programs.h):
; name, entry (0: not a kernel thread), ELF file, size. To add a program,
; put it in user/ and add a PROGRAM line here.

bits 32

; PROGRAM "name" — a table entry, and the file build/user/name.
%macro PROGRAM 1
section .rodata.user_programs
    dd %%name, 0, %%file, %%end - %%file
section .rodata
%%name: db %1, 0
    align 4
%%file: incbin %strcat("build/user/", %1)
%%end:
%endmacro

section .rodata.user_programs
global user_programs
user_programs:

PROGRAM "hello"
PROGRAM "count"
PROGRAM "snoop"
PROGRAM "priv"
PROGRAM "spin"

section .rodata.user_programs
    dd 0, 0, 0, 0               ; the end of the table
