; entry.asm — the kernel's first instructions.
;
; Stage 2 jumps to KERNEL_ADDR (1 MiB) in 32-bit protected mode with
; EAX = BOOT_MAGIC and EBX = the boot_info address. The linker script puts
; section .text.entry first, so _start is the kernel image's first byte.
; C needs two things before kmain can run: a stack, and a zeroed .bss
; (the flat kernel.bin holds no bytes for .bss, so whatever was in that
; memory is still there).

bits 32

global _start
extern kmain
extern __bss_start, __bss_end   ; defined by kernel/linker.ld

KERNEL_STACK_SIZE equ 16384

section .text.entry
_start:
    mov esp, stack_top
    mov ebp, 0                  ; a zero frame pointer ends backtraces
    cld                         ; the ABI wants DF = 0 at every call

    mov edx, eax                ; rep stosb needs eax; keep the magic
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    xor eax, eax
    rep stosb                   ; zero the .bss (our stack is in it, too:
                                ; harmless, nothing is on it yet)

    push ebx                    ; kmain(magic, boot_info): cdecl pushes
    push edx                    ; arguments right to left
    call kmain

    ; kmain should never return. If it does, stop here.
.halt:
    cli
    hlt
    jmp .halt

section .bss
align 16
stack_bottom:
    resb KERNEL_STACK_SIZE
stack_top:
