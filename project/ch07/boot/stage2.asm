; stage2.asm — sprout's second stage loader.
;
; The boot sector loaded us to STAGE2_ADDR and jumped here with DL = boot
; drive, DS = ES = SS = 0 and the stack just below 0x7C00. While we are in
; real mode the BIOS still works, so we use it twice: to ask where the RAM
; is (int 0x15, EAX=0xE820) and to read the kernel from the disk to
; KERNEL_LOAD (int 0x13). Then we open the A20 gate, load a GDT, switch the
; CPU to 32-bit protected mode, copy the kernel up to KERNEL_ADDR (1 MiB)
; and jump to it.

%include "layout.inc"             ; first: org needs STAGE2_ADDR

bits 16
org STAGE2_ADDR

SMAP        equ 0x534D4150      ; "SMAP": the E820 magic, in EAX and EDX
LOAD_CHUNK  equ 64              ; sectors per int 0x13 call: 32 KiB

%ifndef KERNEL_SECTORS
  %error "KERNEL_SECTORS is not defined: build with make"
%elif KERNEL_SECTORS > KERNEL_MAX_SECTORS
  %error "the kernel is too big to load below 0x9F000"
%endif

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
    call e820_summary

    call load_kernel
    jc .disk_error

    call a20_check
    jz .a20_off
    PRINT `sprout: A20 line already enabled\r\n`
    jmp .a20_done
.a20_off:
    call a20_enable
    jc .no_a20
    PRINT `sprout: A20 line enabled\r\n`
.a20_done:

    PRINT `sprout: entering protected mode\r\n`
    cli                         ; no BIOS interrupt handlers after this point
    lgdt [gdt_ptr]
    mov eax, cr0
    or eax, 1                   ; CR0 bit 0: PE, protection enable
    mov cr0, eax
    ; We are in protected mode, but CS still holds a real-mode value.
    ; Only a far jump (or call, or iret) reloads CS: it fetches the code
    ; descriptor and flushes the instructions already decoded as 16-bit.
    jmp CODE_SEL:pm_entry

.no_e820:
    PRINT `sprout: no E820 memory map, giving up\r\n`
    jmp halt16
.disk_error:
    PRINT `sprout: disk read error, status `
    movzx eax, ah
    call print_hex32
    call print_crlf
    jmp halt16
.no_a20:
    PRINT `sprout: cannot enable the A20 line, giving up\r\n`
halt16:
    cli
    hlt
    jmp halt16

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
; e820_summary — one line: how many entries, how much usable RAM. (The
;                kernel prints the whole map.)
;   Clobbers: eax, ecx, edx, si, edi
; ----------------------------------------------------------------------------
e820_summary:
    mov si, E820_MAP
    mov ecx, [BOOT_INFO + BI.e820_count]
    xor edi, edi                ; usable KiB so far
.entry:
    cmp dword [si + E820.type], 1
    jne .next
    mov eax, [si + E820.length]
    mov edx, [si + E820.length + 4]
    shrd eax, edx, 10           ; eax = the 64-bit length / 1024
    add edi, eax
.next:
    add si, E820_size
    dec ecx
    jnz .entry

    PRINT `sprout: E820 memory map: `
    mov eax, [BOOT_INFO + BI.e820_count]
    call print_dec32
    PRINT ` entries, `
    mov eax, edi
    call print_dec32
    PRINT ` KiB usable\r\n`
    ret

; ----------------------------------------------------------------------------
; load_kernel — read KERNEL_SECTORS sectors from KERNEL_LBA to KERNEL_LOAD.
;   Out:      CF=1 and AH = BIOS status on a disk error
;   Clobbers: eax, cx, dx, si
;   Notes:    One int 0x13 call can't fill more than a 64 KiB segment, and
;             many BIOSes refuse more than 127 sectors at once, so we read
;             LOAD_CHUNK sectors at a time, moving the buffer segment on
;             by LOAD_CHUNK * 512 / 16 after each read.
; ----------------------------------------------------------------------------
load_kernel:
    ; Not the sector count: it changes whenever the kernel grows, and
    ; tests/boot.expected should not. make prints it instead.
    PRINT `sprout: loading kernel from LBA `
    mov eax, KERNEL_LBA
    call print_dec32
    PRINT ` to 0x10000\r\n`

    mov cx, KERNEL_SECTORS      ; sectors still to read
.chunk:
    mov ax, LOAD_CHUNK
    cmp cx, ax
    jae .full
    mov ax, cx                  ; the last, partial chunk
.full:
    mov [kernel_dap.count], ax
    push ax
    mov ah, 0x42
    mov dl, [BOOT_INFO + BI.boot_drive]
    mov si, kernel_dap
    int 0x13
    pop dx                      ; dx = sectors just read (pop keeps CF)
    jc .done
    add [kernel_dap.lba], dx
    shl dx, 5                   ; sectors * 512 / 16 = paragraphs
    add [kernel_dap.segment], dx
    shr dx, 5
    sub cx, dx
    jnz .chunk
.done:
    ret

kernel_dap:
    db 16, 0
.count:     dw 0
.offset:    dw 0
.segment:   dw KERNEL_LOAD_SEG
.lba:       dq KERNEL_LBA

%include "a20.inc"
%include "gdt.inc"
%include "print32.inc"

; ----------------------------------------------------------------------------
; pm_entry — first 32-bit instruction. CS = CODE_SEL; every other segment
;            register still holds a real-mode value and must be reloaded.
; ----------------------------------------------------------------------------
bits 32
pm_entry:
    mov ax, DATA_SEL
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, BOOT_ADDR          ; same stack area as before, now 32-bit
    cld

    call pm_con_init
    PM_PRINT `sprout: protected mode; copying kernel to 0x100000\n`

    ; The kernel was linked to run at 1 MiB. Real mode couldn't write
    ; there; now we can.
    mov esi, KERNEL_LOAD
    mov edi, KERNEL_ADDR
    mov ecx, KERNEL_SECTORS * SECTOR_SIZE / 4
    rep movsd

    ; The hand-off, like Multiboot's: a magic number so the kernel knows
    ; who started it, and a pointer to what we found out.
    mov eax, BOOT_MAGIC
    mov ebx, BOOT_INFO
    jmp KERNEL_ADDR             ; a near jump: CS is already CODE_SEL

    ; Pad to exactly STAGE2_SECTORS sectors: that is what the boot sector
    ; reads. If stage 2 outgrows them, NASM stops here with "TIMES value
    ; ... is negative": raise STAGE2_SECTORS in layout.inc.
    times STAGE2_SECTORS * SECTOR_SIZE - ($ - $$) db 0
