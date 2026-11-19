; crt0.asm — where a C user program starts (Chapter 9).
;
; The ELF entry point is _start, not main. The kernel enters it with
; esp = USER_TOP and nothing on the stack, so there is no argc or argv
; yet. Call main, then pass its result to exit: returning from _start
; would pop a return address that isn't there.

bits 32

global _start
extern main, exit

section .text
_start:
    call main
    push eax                    ; exit(main())
    call exit
