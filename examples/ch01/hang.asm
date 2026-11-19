; hang.asm — the smallest boot sector that boots: loop forever.
bits 16
org 0x7C00

    jmp $                       ; jump to this very instruction

    times 510 - ($ - $$) db 0   ; pad to byte 510
    dw 0xAA55                   ; the boot signature: bytes 55 AA
