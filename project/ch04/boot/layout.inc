; layout.inc — where things live, on the disk and in memory.
;
; Shared by boot.asm and stage2.asm (and mirrored in C by the kernel later),
; so both sides of every hand-off agree on the numbers.

%ifndef LAYOUT_INC
%define LAYOUT_INC

; --- disk (512-byte sectors, numbered from 0 = LBA) -------------------------
SECTOR_SIZE     equ 512
STAGE2_LBA      equ 1           ; stage 2 starts right after the boot sector
STAGE2_SECTORS  equ 16          ; and is padded to exactly 8 KiB
KERNEL_LBA      equ STAGE2_LBA + STAGE2_SECTORS     ; 17: the kernel follows
; KERNEL_SECTORS (the kernel's size) comes from the Makefile: nasm -D...

; --- memory (physical addresses) -----------------------------------------------
;   0x00000-0x004FF  real-mode interrupt vectors + BIOS data area: hands off
;   0x00500          BOOT_INFO: what stage 2 found out, for the kernel
;   0x00600          the E820 memory map entries
;   0x07C00          the boot sector (loaded by the BIOS); stack grows down from here
;   0x08000          stage 2 (loaded by the boot sector)
;   0x10000          the kernel, as loaded from disk (real mode can't reach
;                    1 MiB, so stage 2 copies it up after entering protected mode)
;   0x100000         the kernel, where it was linked to run (kernel/linker.ld)
BOOT_INFO       equ 0x0500
E820_MAP        equ 0x0600
E820_MAX        equ 32          ; entries; 32 * 24 bytes ends at 0x0900
BOOT_ADDR       equ 0x7C00
STAGE2_ADDR     equ 0x8000
KERNEL_LOAD_SEG equ 0x1000      ; segment: 0x1000 * 16 = 0x10000
KERNEL_LOAD     equ 0x10000
KERNEL_ADDR     equ 0x100000
KERNEL_MAX_SECTORS equ (0x9F000 - KERNEL_LOAD) / SECTOR_SIZE ; up to the EBDA

; Stage 2 jumps to the kernel with EAX = BOOT_MAGIC and EBX = BOOT_INFO.
BOOT_MAGIC      equ 0x54525053  ; the bytes "SPRT"

; BOOT_INFO layout. The kernel's C struct boot_info must match it.
struc BI
    .boot_drive:    resd 1      ; BIOS drive number we booted from (0x80...)
    .e820_count:    resd 1      ; valid entries at E820_MAP
    .e820_map:      resd 1      ; = E820_MAP
endstruc

; One E820 entry as the BIOS writes it: 24 bytes.
struc E820
    .base:          resq 1
    .length:        resq 1
    .type:          resd 1      ; 1 usable, 2 reserved, 3 ACPI reclaimable, ...
    .acpi:          resd 1      ; ACPI 3.0 extended attributes (bit 0: valid)
endstruc

%endif
