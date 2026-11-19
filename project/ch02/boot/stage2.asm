; stage2.asm — sprout's second stage loader (16-bit real mode, for now).
;
; The boot sector loaded us to STAGE2_ADDR and jumped here with DL = boot
; drive, DS = ES = SS = 0 and the stack just below 0x7C00. With 8 KiB of
; room we can afford to ask the BIOS questions the kernel will need
; answered, because once we leave real mode the BIOS is gone. This chapter
; asks the most important one: where is the RAM? (int 0x15, EAX=0xE820)

%include "layout.inc"             ; first: org needs STAGE2_ADDR

bits 16
org STAGE2_ADDR

SMAP        equ 0x534D4150      ; "SMAP": the E820 magic, in EAX and EDX

stage2:
    jmp main                    ; skip over the included helper functions

%include "print16.inc"

main:
    movzx edx, dl
    mov [BOOT_INFO + BI.boot_drive], edx
    mov dword [BOOT_INFO + BI.e820_map], E820_MAP

    PRINT `sprout: stage 2 running at 0x8000\r\n`

    call e820_detect
    jc .no_e820
    call e820_print
    jmp halt
.no_e820:
    PRINT `sprout: no E820 memory map, giving up\r\n`

halt:
    PRINT `sprout: stage 2 done, halting\r\n`
.forever:
    cli
    hlt
    jmp .forever

; ----------------------------------------------------------------------------
; e820_detect — fill E820_MAP with the BIOS memory map.
;   Out:      BOOT_INFO.e820_count = entries stored; CF=1 if E820 failed
;   Clobbers: eax, ebx, ecx, edx, di, bp
;   Notes:    Each int 0x15 call returns one entry and a continuation value
;             in EBX for the next call; EBX = 0 (or CF=1 after the first
;             entry) means that was the last one. The BIOS may return 20
;             bytes (no ACPI field), so we preset that field to 1 ("valid").
; ----------------------------------------------------------------------------
e820_detect:
    mov di, E820_MAP            ; ES:DI = where the BIOS writes the entry
    xor ebx, ebx                ; continuation: 0 = start from the beginning
    xor bp, bp                  ; entries stored
.next:
    mov dword [di + E820.acpi], 1
    mov eax, 0xE820
    mov edx, SMAP
    mov ecx, E820_size          ; 24: we can take the ACPI 3.0 field
    int 0x15
    jc .end                     ; CF=1: no more entries (or no E820 at all)
    cmp eax, SMAP               ; the BIOS echoes "SMAP" if it understood
    jne .fail
    mov eax, [di + E820.length]
    or eax, [di + E820.length + 4]
    jz .skip                    ; ignore empty entries
    inc bp
    add di, E820_size
    cmp bp, E820_MAX
    je .done                    ; table full: keep what we have
.skip:
    test ebx, ebx
    jnz .next
    jmp .done
.end:
    test bp, bp                 ; CF on the very first call: no E820
    jz .fail
.done:
    movzx eax, bp
    mov [BOOT_INFO + BI.e820_count], eax
    clc
    ret
.fail:
    stc
    ret

; ----------------------------------------------------------------------------
; e820_print — print the stored map, one entry per line, and the total of
;              the usable entries in KiB.
;   Clobbers: eax, ecx, edx, si, edi
; ----------------------------------------------------------------------------
e820_print:
    PRINT `sprout: E820 memory map (`
    mov eax, [BOOT_INFO + BI.e820_count]
    call print_dec32
    PRINT ` entries)\r\n`
    PRINT `  base                length              type\r\n`
    mov si, E820_MAP
    mov ecx, [BOOT_INFO + BI.e820_count]
    xor edi, edi                ; usable KiB so far
.entry:
    PRINT `  `
    call print_hex64            ; base, at [si]
    PRINT `  `
    add si, E820.length
    call print_hex64
    sub si, E820.length
    PRINT `  `
    mov eax, [si + E820.type]
    call print_dec32
    call print_type_name
    call print_crlf
    cmp dword [si + E820.type], 1
    jne .not_usable
    mov eax, [si + E820.length]
    mov edx, [si + E820.length + 4]
    shrd eax, edx, 10           ; eax = the 64-bit length / 1024
    add edi, eax
.not_usable:
    add si, E820_size
    dec ecx
    jnz .entry

    PRINT `sprout: usable memory: `
    mov eax, edi
    call print_dec32
    PRINT ` KiB\r\n`
    ret

; ----------------------------------------------------------------------------
; print_type_name — " usable", " reserved", ... for the E820 type in eax.
; ----------------------------------------------------------------------------
print_type_name:
    push si
    cmp eax, 5
    jbe .known
    xor eax, eax                ; anything else: "unknown"
.known:
    mov si, [type_names + eax * 2]
    call print
    pop si
    ret

type_names:
    dw .unknown, .usable, .reserved, .acpi_reclaim, .acpi_nvs, .bad
.unknown:       db " unknown", 0
.usable:        db " usable", 0
.reserved:      db " reserved", 0
.acpi_reclaim:  db " ACPI reclaimable", 0
.acpi_nvs:      db " ACPI NVS", 0
.bad:           db " bad memory", 0

    ; Pad to exactly STAGE2_SECTORS sectors: that is what the boot sector
    ; reads. If stage 2 outgrows them, NASM stops here with "TIMES value
    ; ... is negative": raise STAGE2_SECTORS in layout.inc.
    times STAGE2_SECTORS * SECTOR_SIZE - ($ - $$) db 0
