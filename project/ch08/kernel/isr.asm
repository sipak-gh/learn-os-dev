; isr.asm — the first instructions of every interrupt and exception.
;
; The CPU enters an interrupt gate with interrupts disabled and pushes, on
; the current stack: EFLAGS, CS, EIP, and for some exceptions an error code.
; It does not say *which* vector fired, and C cannot save registers or
; return with iret. So every vector gets a tiny stub that pushes its number
; (and a dummy error code where the CPU pushes none, so the stack looks the
; same for all), then jumps to one common routine that saves everything,
; calls interrupt_dispatch(struct regs *) in C, restores everything and
; returns with iret. struct regs in interrupts.h mirrors this layout.

bits 32

extern interrupt_dispatch

KERNEL_DATA_SEL equ 0x10

; ISR_NOERR n — a vector where the CPU pushes no error code.
%macro ISR_NOERR 1
isr_%1:
    push dword 0                ; dummy error code
    push dword %1               ; vector number
    jmp isr_common
%endmacro

; ISR_ERR n — a vector where the CPU has already pushed an error code.
%macro ISR_ERR 1
isr_%1:
    push dword %1
    jmp isr_common
%endmacro

section .text

; Exceptions 0-31. The ones with an error code: 8, 10-14, 17, 21 (Intel
; SDM Vol. 3A, table 6-1 "Exceptions and Interrupts").
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_ERR   21
%assign v 22
%rep 10
ISR_NOERR v
%assign v v + 1
%endrep

; Hardware interrupts: the PICs deliver IRQ 0-15 as vectors 32-47.
%rep 16
ISR_NOERR v
%assign v v + 1
%endrep

; ----------------------------------------------------------------------------
; isr_common — save the interrupted state, call C, restore, iret.
;   In:       stack = vector, error code, then what the CPU pushed
;   Notes:    The data segment registers may hold anything (later: user
;             selectors), so we save them and load the kernel's.
; ----------------------------------------------------------------------------
isr_common:
    pusha                       ; eax ecx edx ebx esp(ignored) ebp esi edi
    push ds
    push es
    push fs
    push gs
    mov ax, KERNEL_DATA_SEL
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    cld                         ; C code assumes DF = 0

    push esp                    ; argument: pointer to the saved registers
    call interrupt_dispatch
    add esp, 4

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8                  ; drop vector and error code
    iret

; The stubs' addresses, for idt.c: isr_stubs[v] is vector v's entry point.
section .rodata
global isr_stubs
isr_stubs:
%assign v 0
%rep 48
    dd isr_ %+ v
%assign v v + 1
%endrep
