; usermode.asm — the way down to ring 3 (Chapter 9).
;
; void enter_user_mode(uint32_t eip, uint32_t esp);
;
; There is no instruction that just lowers the privilege level. iret
; does it: when the CS it pops has RPL 3, the CPU drops to ring 3 and
; pops SS and ESP as well. So we build the frame an interrupt from ring 3
; would have left on the stack, and return from an interrupt that never
; happened.

bits 32

USER_CS     equ 0x18 | 3        ; kernel/gdt.h
USER_DS     equ 0x20 | 3
EFLAGS_IF   equ 0x200           ; interrupts on in user mode
EFLAGS_1    equ 0x002           ; bit 1 is always 1

section .text

global enter_user_mode
enter_user_mode:
    mov ecx, [esp + 4]          ; eip
    mov edx, [esp + 8]          ; esp

    mov ax, USER_DS             ; the data segments the program will use
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push USER_DS                ; ss
    push edx                    ; esp
    push EFLAGS_IF | EFLAGS_1   ; eflags (IOPL 0: no I/O ports, no cli)
    push USER_CS                ; cs
    push ecx                    ; eip

    ; Don't hand kernel values to the program.
    xor eax, eax
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx
    xor esi, esi
    xor edi, edi
    xor ebp, ebp
    iret
