; nosig.asm — the same, but with the signature forgotten. The BIOS refuses it.
bits 16
org 0x7C00

    jmp $

    times 512 - ($ - $$) db 0   ; 512 bytes, last two are 00 00
