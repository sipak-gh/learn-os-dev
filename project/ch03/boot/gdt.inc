; gdt.inc — the Global Descriptor Table stage 2 needs for protected mode.
;
; In protected mode a segment register holds a *selector*: an index into
; this table (times 8) plus two flag bits. The descriptor it selects gives
; the segment's base, limit and access rights. We use the "flat" model:
; code and data both start at 0 and span all 4 GiB, so an address is just
; an address. Paging (Chapter 7) will do the real protection work.

%ifndef GDT_INC
%define GDT_INC

; Selectors: byte offset of the descriptor in the table. The low 3 bits
; (table indicator, requested privilege level) are 0: GDT, ring 0.
CODE_SEL    equ gdt.code - gdt      ; 0x08
DATA_SEL    equ gdt.data - gdt      ; 0x10

; GDT_ENTRY base, limit, access, flags — one 8-byte descriptor. The
; fields are scattered for 80286 compatibility; this macro gathers them.
;   access: bit 7 present, bits 5-6 privilege level (DPL), bit 4 = 1 for
;           code/data, bit 3 executable, bit 1 readable (code) or
;           writable (data)
;   flags:  bit 3 granularity (limit in 4 KiB pages), bit 2 = 32-bit
%macro GDT_ENTRY 4
    dw (%2) & 0xFFFF                        ; limit bits 0-15
    dw (%1) & 0xFFFF                        ; base bits 0-15
    db ((%1) >> 16) & 0xFF                  ; base bits 16-23
    db (%3)                                 ; access byte
    db (((%4) & 0x0F) << 4) | (((%2) >> 16) & 0x0F) ; flags, limit 16-19
    db ((%1) >> 24) & 0xFF                  ; base bits 24-31
%endmacro

    align 8
gdt:
.null:  dq 0                                ; entry 0 must be unused
.code:  GDT_ENTRY 0, 0xFFFFF, 0x9A, 0xC     ; ring 0 code: execute/read
.data:  GDT_ENTRY 0, 0xFFFFF, 0x92, 0xC     ; ring 0 data: read/write
.end:

; What lgdt loads: the table's size minus one, then its linear address.
gdt_ptr:
    dw gdt.end - gdt - 1
    dd gdt

%endif
