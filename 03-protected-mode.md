# Chapter 3 — Protected Mode

Stage 2 runs in real mode. That gives it 16-bit segments, a 1 MiB address space, and the BIOS
as its only way to reach the hardware. The rest of sprout needs much more than that. In this
chapter stage 2 switches the CPU to 32-bit protected mode in five steps: it makes sure the A20
address line works, describes its segments in a Global Descriptor Table (GDT), sets bit 0 of
CR0, and far-jumps into 32-bit code. Then it prints by writing straight into the VGA text
buffer, because the BIOS cannot be called any more. At the end stage 2 prints its own CR0 and
segment registers, and it reads back a value it wrote above 1 MiB, an address that was out of
reach a moment earlier.

Previous: [Chapter 2](02-loading-more-code.md). Next: [Chapter 4](04-hello-from-c.md).

## Why leave real mode

Every x86 CPU starts in real mode, so it behaves like an 8086 from 1978. Chapter 1 covered the
rules: an address is `segment * 16 + offset`, both parts are 16 bits, and the result is a 20-bit
physical address. That has three consequences an operating system cannot live with:

- **1 MiB, at most.** `0xFFFF * 16 + 0xFFFF` is `0x10FFEF`. Everything above that is out of
  reach. QEMU gave stage 2 128 MiB of RAM, and the E820 map from Chapter 2 shows 127 MiB of it
  sitting above 1 MiB.
- **64 KiB windows.** An offset is 16 bits, so one segment covers 64 KiB. Anything bigger
  needs segment arithmetic.
- **No protection.** Any code can write anywhere, including the interrupt vector table at
  address 0, the BIOS data area, and the kernel itself. There is no "user" and no "kernel".

Protected mode fixes all three. It first appeared on the 80286 and was extended on the 80386
(see the history section at the end). In 32-bit protected mode, offsets are 32 bits, so one
segment can cover all 4 GiB. A segment register no longer holds a number to multiply by 16.
It holds a *selector*, an index into a table of *descriptors* that the operating system owns.
Each descriptor sets the segment's base, size and privilege level. Later chapters build on
this mode: interrupts through an IDT (Chapter 5), paging (Chapter 7), and ring 3 for user
programs (Chapter 9).

The cost is the BIOS. Its services (`int 0x10`, `int 0x13`, `int 0x15`) are 16-bit real-mode
code that expects real-mode segments, so a protected-mode program cannot call them. That is why
stage 2 asks for the memory map *before* it switches, and why Chapter 4 will load the kernel
from disk before the switch too. After this chapter, sprout writes its own drivers.

| | real mode | 32-bit protected mode |
|---|---|---|
| address | `segment * 16 + offset` | descriptor base + 32-bit offset |
| reachable memory | 1 MiB (+ 64 KiB, see A20) | 4 GiB |
| segment register holds | a paragraph number | a selector (index into the GDT) |
| protection | none | limits, privilege levels 0–3 |
| BIOS calls | yes | no |
| default operand size | 16 bits | 32 bits (in a 32-bit code segment) |

The switch itself is short. Before it can happen, the A20 line has to work.

## The A20 line

The 8086 had 20 address lines, A0 to A19. A segment:offset pair can describe addresses up to
`0x10FFEF`, but bit 20 had no pin to go out on, so the 8086 dropped it. `FFFF:0010` is
`0xFFFF0 + 0x10 = 0x100000`, and on an 8086 that became address 0. Some DOS-era programs relied
on this wrap-around.

The 80286 in IBM's PC/AT (1984) had 24 address lines, so `FFFF:0010` really reached 1 MiB, even
in real mode. That broke the programs that relied on the wrap. IBM's fix was a gate on address
line 20. A spare output pin of the 8042 keyboard controller was wired into A20. While the gate
is closed, bit 20 of every physical address is forced to 0, so the machine wraps like an 8086.
While it is open, memory behaves normally.

This matters in protected mode too. With the gate closed, bit 20 is still forced to 0, so
`0x100000` reads `0x000000`, `0x300000` reads `0x200000`, and so on: every odd megabyte is a
mirror of the even one below it. A kernel loaded at 1 MiB with the gate closed would overwrite
the interrupt vector table.

### Testing it

If the gate is closed, two addresses exactly 1 MiB apart are the same byte. Stage 2 picks a
pair it can reach from real mode:

| segment:offset | arithmetic | physical (A20 open) | physical (A20 closed) |
|---|---|---|---|
| `0000:0500` | `0x00000 + 0x0500` | `0x000500` | `0x000500` |
| `FFFF:0510` | `0xFFFF0 + 0x0510` | `0x100500` | `0x000500` |

It writes `0x00` to the first address and `0xFF` to the second, then reads the first one back.
If the result is `0xFF`, both writes went to the same byte, so the addresses wrap. `0x500` is
`BOOT_INFO` from Chapter 2, and stage 2 has already stored the boot drive there, so the test
must put both bytes back.

### Opening the gate

There are three classic methods, from the friendliest to the most direct:

1. **Ask the BIOS:** `int 0x15` with `AX=0x2401`. This works on many BIOSes, and the result is
   reported in CF.
2. **The 8042 keyboard controller**, the original AT method. Wait until the controller's input
   buffer is empty (bit 1 of status port `0x64`), send command `0xD1` ("write output port") to
   port `0x64`, wait again, then write the new output port value (with bit 1, the A20 gate,
   set) to port `0x60`. The value used in practice is `0xDF`. xv6's boot sector does exactly
   this. It is slow and has many steps, and a careless value can reset the machine.
3. **"Fast A20":** bit 1 of system control port `0x92`. Read the port, set bit 1, and write it
   back. **Bit 0 of the same port resets the CPU**, so always clear it before writing.

The project file:

`project/ch03/boot/a20.inc`
```nasm
; a20.inc — make sure address line 20 works (16-bit real mode code).
;
; The 8086 had 20 address lines; FFFF:0010 wrapped around to 0000:0000 and
; some DOS programs depended on it. The IBM AT (80286, 24 lines) kept the
; wrap by gating line 20 through a spare pin of the keyboard controller:
; with the "A20 gate" closed, bit 20 of every address is forced to 0, so
; 1 MiB + x reads x. We want all memory, so the gate must be open.

%ifndef A20_INC
%define A20_INC

; ----------------------------------------------------------------------------
; a20_check — is the A20 line enabled?
;   Out:      ZF=0 (jnz) if enabled, ZF=1 (jz) if addresses wrap at 1 MiB
;   Clobbers: nothing but flags
;   Notes:    0000:0500 is physical 0x000500 and FFFF:0510 is physical
;             0x100500. Write different bytes to both; if the first one
;             changes when we write the second, they are the same byte.
; ----------------------------------------------------------------------------
a20_check:
    pushf                       ; we need IF back as it was (cli below)
    push ds
    push es
    push di
    push si
    push ax
    cli
    xor ax, ax
    mov es, ax
    mov di, 0x0500              ; es:di = 0x000500
    not ax
    mov ds, ax
    mov si, 0x0510              ; ds:si = 0x100500 (or 0x000500 if wrapping)
    mov al, [es:di]             ; save both bytes
    mov ah, [ds:si]
    push ax
    mov byte [es:di], 0x00
    mov byte [ds:si], 0xFF
    cmp byte [es:di], 0xFF      ; ZF=1: the write went to 0x000500
    pop ax
    mov [ds:si], ah             ; restore (in this order: if they alias,
    mov [es:di], al             ; the low byte's old value wins)
    pop ax                      ; pop and mov keep flags; only popf would
    pop si                      ; change them, so we pass ZF out through
    pop di                      ; the flags image on the stack:
    pop es
    pop ds
    jz .wraps
    popf
    or sp, sp                   ; sp is never 0: ZF=0, "enabled"
    ret
.wraps:
    popf
    cmp sp, sp                  ; ZF=1, "wraps"
    ret

; ----------------------------------------------------------------------------
; a20_enable — open the A20 gate if it is closed, trying the BIOS first
;              and then the "fast A20" bit in system control port 0x92.
;   Out:      CF=0 if A20 is enabled now, CF=1 if every method failed
;   Clobbers: ax
;   Notes:    The third classic method talks to the 8042 keyboard
;             controller (command 0xD1, write output port, bit 1). QEMU,
;             like most machines since the 1990s, has port 0x92, so we
;             leave the 8042 dance to the chapter text.
; ----------------------------------------------------------------------------
a20_enable:
    call a20_check
    jnz .on

    mov ax, 0x2401              ; int 0x15, AX=0x2401: BIOS, enable A20
    int 0x15
    call a20_check
    jnz .on

    in al, 0x92                 ; port 0x92 bit 1: A20 gate (bit 0 would
    test al, 2                  ; reset the machine: never set it here)
    jnz .no_fast
    or al, 2
    and al, 0xFE
    out 0x92, al
.no_fast:
    call a20_check
    jnz .on
    stc
    ret
.on:
    clc
    ret

%endif
```

- `a20_check` reports its answer in ZF, not in a register, so the caller can write
  `call a20_check` / `jz .a20_off`. The tricky part is the exit path. The pops after the `cmp`
  leave the flags alone, but `popf` restores the *caller's* flags, which would overwrite ZF.
  So the function branches first, and after `popf` it sets ZF again with an instruction whose
  result it knows: `or sp, sp` gives ZF=0 because SP is never 0 here, and `cmp sp, sp` gives
  ZF=1.
- It saves and restores both bytes. If the two addresses are the same byte, the restore order
  matters. The low address is written last, so its original value is what remains.
- `cli` keeps an interrupt handler from running between the two writes and touching either
  byte. `pushf`/`popf` put IF back the way the caller had it.
- `a20_enable` tries each method and checks again after each one. A method can fail silently,
  so the check is the only proof that the gate is open.

In `stage2.asm`, after the memory map, `call a20_check` / `jz .a20_off` picks between printing
`sprout: A20 line already enabled` and calling `a20_enable` (which prints `A20 line enabled`,
or gives up on CF=1). On QEMU, SeaBIOS has already opened the gate before it jumps to the boot
sector, so sprout prints "already enabled". On a real machine the other branch may run.

### Example: closing the gate on purpose

This example boot sector closes the gate with port `0x92` and runs the same test after each
step. It uses `com1.inc`, the serial helpers from Chapter 1's examples, copied into this
directory.

`examples/ch03/a20.asm`
```nasm
; a20.asm — watch the A20 gate work: close it, see memory wrap at 1 MiB,
; open it again. Uses the "fast A20" bit 1 of system control port 0x92.
bits 16
org 0x7C00

start:
    jmp 0:.go
.go:
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov sp, 0x7C00

    call report                 ; as the BIOS left it

    in al, 0x92
    and al, ~2 & 0xFE           ; clear bit 1 (A20); never set bit 0 (reset)
    out 0x92, al
    mov si, msg_closed
    call puts
    call report

    in al, 0x92
    or al, 2
    and al, 0xFE
    out 0x92, al
    mov si, msg_opened
    call puts
    call report

    cli
    hlt

; report: write 0x00 to 0x000500 and 0xFF to 0x100500 (FFFF:0510), then
; print what 0x000500 holds. 0xFF means both addresses are the same byte.
report:
    push es
    mov ax, 0xFFFF
    mov es, ax
    mov byte [0x0500], 0x00
    mov byte [es:0x0510], 0xFF
    mov si, msg_read
    call puts
    mov al, [0x0500]
    call puthex8
    cmp byte [0x0500], 0xFF
    mov si, msg_wraps
    je .print
    mov si, msg_ok
.print:
    call puts
    pop es
    ret

%include "com1.inc"

msg_closed: db "closed the A20 gate (port 0x92 bit 1 = 0)", 13, 10, 0
msg_opened: db "opened the A20 gate (port 0x92 bit 1 = 1)", 13, 10, 0
msg_read:   db "  byte at 0x000500 after writing 0xFF to 0x100500: 0x", 0
msg_wraps:  db " -> addresses wrap at 1 MiB", 13, 10, 0
msg_ok:     db " -> A20 enabled", 13, 10, 0

    times 510 - ($ - $$) db 0
    dw 0xAA55
```

The Makefile in `examples/ch03/` works like Chapter 1's. `make serial NAME=x` boots `x.asm`
headless for 3 seconds under `timeout` and prints COM1. It also takes `QEMU_D=` for QEMU's `-d`
logging flags.

```console
$ cd examples/ch03
$ make serial NAME=a20
timeout 3 qemu-system-i386  -drive format=raw,file=build/a20.img -no-reboot \
    -display none -serial stdio -monitor none
  byte at 0x000500 after writing 0xFF to 0x100500: 0x00 -> A20 enabled
closed the A20 gate (port 0x92 bit 1 = 0)
  byte at 0x000500 after writing 0xFF to 0x100500: 0x00 -> A20 enabled
opened the A20 gate (port 0x92 bit 1 = 1)
  byte at 0x000500 after writing 0xFF to 0x100500: 0x00 -> A20 enabled
qemu-system-i386: terminating on signal 15 from pid 425056 (timeout)
make: [Makefile:25: serial] Error 124 (ignored)
```

The middle line should say "addresses wrap at 1 MiB", and on this machine it does not. QEMU
did see the port write. Its `-d mmu` log records every change to the A20 gate (other log lines
removed):

```console
$ make serial NAME=a20 QEMU_D="-d mmu"
  byte at 0x000500 after writing 0xFF to 0x100500: 0x00 -> A20 enabled
A20 update: a20=0
closed the A20 gate (port 0x92 bit 1 = 0)
  byte at 0x000500 after writing 0xFF to 0x100500: 0x00 -> A20 enabled
A20 update: a20=1
opened the A20 gate (port 0x92 bit 1 = 1)
  byte at 0x000500 after writing 0xFF to 0x100500: 0x00 -> A20 enabled
```

- The program is right, and the emulator is lax. The course uses QEMU 6.2. In its source
  (`target/i386/tcg/sysemu/excp_helper.c`, tag `v6.2.0`), the TCG CPU emulator translates
  addresses with paging off as `paddr = addr`, so the A20 mask is applied only while paging is
  on. Later versions apply the mask on the no-paging path too. On a real PC with the gate
  closed, the middle report prints `0xFF -> addresses wrap at 1 MiB`. I could not run that
  here, and newer QEMU versions are covered only by reading their source.
- The `timeout` message and `Error 124 (ignored)` are expected. The example ends with `hlt`,
  so QEMU never exits by itself, and the Makefile's `-timeout` stops it.
- The `-d mmu` log also contains 455 lines like `CR0 update: CR0=0x60000010` before the boot
  sector runs. SeaBIOS itself switches between real and protected mode many times while it
  sets up the machine. That first value, `0x60000010`, is CR0's reset value.

The lesson for an OS developer is that an emulator is a model of the hardware, not the
hardware. sprout checks the gate, and that code is right for real machines, even though QEMU
6.2 would never notice if it were missing.

## Segments become selectors: the GDT

In protected mode a segment register holds a 16-bit **selector**:

```text
 15                                3   2   1 0
+-----------------------------------+----+-----+
|  index into the descriptor table  | TI | RPL |
+-----------------------------------+----+-----+
```

- **index**: which 8-byte descriptor to use. Because every descriptor is 8 bytes, a selector
  whose low 3 bits are 0 is also the descriptor's byte offset in the table: selector `0x08` is
  entry 1, `0x10` is entry 2.
- **TI**: 0 = the Global Descriptor Table, 1 = a Local Descriptor Table (which sprout never
  uses).
- **RPL**: the requested privilege level. It is 0 here, and it matters in Chapter 9.

When a segment register is loaded, the CPU reads the descriptor from the GDT, checks it, and
copies base, limit and access rights into a hidden part of the register, the descriptor cache.
Every memory access then uses that cache: `linear = base + offset`, with the offset checked
against the limit. The table is read again only when the register is reloaded. Remember that
rule, because the switch depends on it.

### One descriptor, eight bytes

```text
byte 0-1   limit bits 15..0
byte 2-4   base bits 23..0
byte 5     access byte:  P | DPL (2 bits) | S | E | DC | RW | A
byte 6     high nibble: flags G | D/B | L | AVL    low nibble: limit bits 19..16
byte 7     base bits 31..24
```

The base is 32 bits and the limit is 20 bits, both split into pieces. This is a leftover from
the 80286. Its descriptors were also 8 bytes, but held only a 24-bit base and a 16-bit limit,
and the last two bytes were reserved. The 386 kept that layout and put the extra base and limit
bits, plus a nibble of flags, into the reserved bytes.

The **access byte**:

| bit | name | meaning | code 0x9A | data 0x92 |
|---|---|---|---|---|
| 7 | P | present | 1 | 1 |
| 6–5 | DPL | privilege level of the segment (0 = kernel, 3 = user) | 00 | 00 |
| 4 | S | 1 = code or data segment (0 = system: TSS, gates) | 1 | 1 |
| 3 | E | executable | 1 | 0 |
| 2 | DC | conforming (code) / grows down (data) | 0 | 0 |
| 1 | RW | readable (code) / writable (data) | 1 | 1 |
| 0 | A | accessed, set by the CPU | 0 | 0 |

`1001 1010` is `0x9A` and `1001 0010` is `0x92`. Chapter 9 will add the ring 3 versions,
`0xFA` and `0xF2`, which differ only in DPL.

The **flags** nibble:

| bit | name | meaning | value `0xC` |
|---|---|---|---|
| 3 | G | granularity: limit counts 4 KiB pages instead of bytes | 1 |
| 2 | D/B | default size: 32-bit code / 32-bit stack | 1 |
| 1 | L | 64-bit code (long mode only) | 0 |
| 0 | AVL | free for the OS | 0 |

With G=1, a limit of `0xFFFFF` means `0xFFFFF` pages of 4 KiB, so the segment ends at
`0xFFFFFFFF`, the full 4 GiB.

### The flat model

sprout uses the simplest possible table, the one most 32-bit kernels use: a code segment and a
data segment, both with base 0 and limit 4 GiB. With base 0, `linear = 0 + offset`, so an
address in a register *is* the linear address, and segmentation stops getting in the way. C
compilers assume exactly this. Protection will come from paging in Chapter 7, not from segment
limits.

Entry 0 must be the null descriptor. The CPU lets you load selector 0 into DS or ES, but any
access through it faults. That is useful for catching uninitialised segment registers.

`project/ch03/boot/gdt.inc`
```nasm
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
```

- `GDT_ENTRY` takes the four values people think in (base, limit, access, flags) and scatters
  them the way the CPU wants. Writing the 8 bytes by hand works too (the triple-fault example
  below does it with `dq 0x00CF9A000000FFFF`), but it is easy to get wrong.
- The selectors are computed from labels, so inserting an entry cannot leave a stale constant
  behind.
- `align 8` is for speed. The CPU does not require it.
- `gdt_ptr` is the 6-byte operand of `lgdt`: a 16-bit limit, which is the table size *minus
  one* (here `3 * 8 - 1 = 23`), then the 32-bit linear address of the table. The GDT can hold
  up to 8192 entries (64 KiB).

Here are the bytes NASM produced. Stage 2 is loaded at `0x8000`, so `xxd -o 0x8000` shows
memory addresses:

```console
$ cd project/ch03
$ make
mkdir -p build
nasm -f bin -w+all -w-reloc -I boot/ -MD build/boot.d -MP -l build/boot.lst -o build/boot.bin boot/boot.asm
nasm -f bin -w+all -w-reloc -I boot/ -MD build/stage2.d -MP -l build/stage2.lst -o build/stage2.bin boot/stage2.asm
cat build/boot.bin build/stage2.bin > build/sprout.img
truncate -s 1M build/sprout.img
$ xxd -s 0x448 -l 30 -g 1 -c 8 -o 0x8000 build/stage2.bin
00008448: 00 00 00 00 00 00 00 00  ........
00008450: ff ff 00 00 00 9a cf 00  ........
00008458: ff ff 00 00 00 92 cf 00  ........
00008460: 17 00 48 84 00 00        ..H...
```

Here is the code descriptor at `0x8450` decoded:

| bytes | field | value |
|---|---|---|
| `ff ff` | limit 15..0 | `0xFFFF` |
| `00 00 00` | base 23..0 | `0x000000` |
| `9a` | access | present, ring 0, code, readable |
| `cf` | flags `c` (G=1, D=1), limit 19..16 `f` | limit `0xFFFFF` pages = 4 GiB |
| `00` | base 31..24 | base = 0 |

The pointer at `0x8460` is `17 00` (limit 23) followed by `48 84 00 00`. The base is `0x8448`,
the address of the table, which is in memory at that address because stage 2 runs there.
`lgdt` needs the *linear* address. In real mode with DS = 0 that is the same as the offset,
which is why stage 2 keeps DS = 0.

## The switch

With A20 open and a table ready, the switch takes six instructions:

`project/ch03/boot/stage2.asm` (part)
```nasm
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
```

Each step has a reason:

1. **`cli`.** In protected mode the CPU finds interrupt handlers through the IDTR, which still
   describes the real-mode vector table at address 0 (base 0, limit `0x3FF`). Those bytes are
   not valid protected-mode gates. A timer tick a microsecond after the switch would crash the
   machine, as the triple-fault example below shows. Interrupts stay off until Chapter 5
   builds an IDT.
2. **`lgdt [gdt_ptr]`.** This loads GDTR, the register holding the table's base and limit. It
   works in real mode, and nothing reads the table until a segment register is loaded in
   protected mode.
3. **`mov eax, cr0` / `or eax, 1` / `mov cr0, eax`.** Bit 0 of control register 0 is PE,
   "protection enable". Use read-modify-write, because the other bits mean things too (bit 4,
   ET, is already set; see below).
4. **`jmp CODE_SEL:pm_entry`.** This is a far jump, which loads CS with selector `0x08`.

The far jump is the step people forget. Right after `mov cr0, eax` the CPU *is* in protected
mode, but CS's descriptor cache still holds what real mode put there: base 0, limit 64 KiB,
16-bit. The cache is refreshed only when CS is reloaded, and the default operand size comes
from the cache, so the CPU keeps decoding instructions as 16-bit code. `mov cs, ax` does not
exist. CS changes only through a far `jmp`, `call`, `ret` or `iret`. The far jump loads
selector `0x08`, so the CPU reads descriptor 1, sees D=1, and decodes everything from
`pm_entry` on as 32-bit code. On older CPUs the jump also flushed the prefetch queue, which
held instructions already decoded the old way.

`ndisasm` shows both sides. Decoded as 16-bit code at stage 2's address:

```console
$ ndisasm -b 16 -o 0x8000 build/stage2.bin | sed -n "/0000817F/,/00008195/p"
0000817F  FA                cli
00008180  0F01166084        lgdt word [0x8460]
00008185  0F20C0            mov eax,cr0
00008188  6683C801          or eax,0x1
0000818C  0F22C0            mov cr0,eax
0000818F  EA8B850800        jmp word 0x8:word 0x858b
00008194  56                push si
00008195  EB28              jmp 0x81bf
```

- `EA` is the far jump opcode. It is followed by a 16-bit offset, `8B 85` = `0x858B`, and the
  16-bit selector `08 00`. The jump is assembled as 16-bit code because it still executes in
  a 16-bit code segment.
- `lgdt word [0x8460]` reads `gdt_ptr` at `0x8460`, matching the `xxd` dump above.
- `or eax, 0x1` needs the `66` operand-size prefix, because 32-bit registers are the exception
  in 16-bit code. The control-register moves (`0F 20`, `0F 22`) are always 32-bit.

`pm_entry` lives at `0x858B`, after the included files. The right way to read it is as 32-bit
code:

```console
$ ndisasm -b 32 -o 0x858b -e 0x58b build/stage2.bin | head -8
0000858B  66B81000          mov ax,0x10
0000858F  8ED8              mov ds,eax
00008591  8EC0              mov es,eax
00008593  8EE0              mov fs,eax
00008595  8EE8              mov gs,eax
00008597  8ED0              mov ss,eax
00008599  BC007C0000        mov esp,0x7c00
0000859E  FC                cld
```

The same bytes decoded as 16-bit code are a different program:

```console
$ ndisasm -b 16 -o 0x858b -e 0x58b build/stage2.bin | head -3
0000858B  66B810008ED8      mov eax,0xd88e0010
00008591  8EC0              mov es,ax
00008593  8EE0              mov fs,ax
```

In 32-bit code, `66` makes `mov ax, 0x10` a 16-bit move. In 16-bit code the same prefix makes
it a 32-bit move, which swallows the next two bytes (`8E D8`, the `mov ds`) as part of the
immediate. This is what happens without the far jump: the CPU runs these bytes the second way,
and the program goes off the rails within a few instructions. Exercise 2 makes you watch it
happen.

### `pm_entry`: the first 32-bit instructions

`project/ch03/boot/stage2.asm` (part)
```nasm
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
```

- `bits 32` tells NASM to assemble for a 32-bit code segment from here on. It changes nothing
  at run time. It must match the D flag of the segment the code will run in. NASM cannot check
  that for you.
- Only CS was reloaded by the jump. DS, ES, FS, GS and SS still have real-mode descriptor
  caches (base 0, limit 64 KiB). They would work by accident for small addresses, but on a real
  CPU the first access above 64 KiB, such as the write to `0x100000` below, would fault. Loading
  `DATA_SEL` into each one gives them the flat 4 GiB data descriptor.
- The new stack starts at `0x7C00` again, and now ESP is 32 bits.
- `cld` sets the string direction for `rep movsd` and `lodsb`, which the print code uses.
  Never assume the flags someone else left.

## No BIOS any more: VGA text and COM1

Up to now every character went through `int 0x10` (screen) and the COM1 routines in
`print16.inc`. The serial code was already direct port I/O, so it works in protected mode with
the same ports. The screen is new: in protected mode you write the characters into video memory
yourself.

### The text buffer at 0xB8000

In the 80×25 colour text mode the BIOS leaves us in, the screen is an array of 2000 16-bit
cells at physical address `0xB8000`, 4000 bytes in all. Row by row, each cell is two bytes:

```text
address = 0xB8000 + 2 * (row * 80 + col)

  byte 0: character (code page 437)     byte 1: attribute
                                         bit 7    6 5 4    3 2 1 0
                                         blink  background foreground
```

The 16 colours are the CGA palette: 0 black, 1 blue, 2 green, 4 red, 7 light grey, 8 dark grey,
`0xA` light green, `0xF` white, and so on. Bit 3 of the foreground makes a colour bright.
`0x07` is the BIOS's grey on black, `0x0A` is light green on black, and `0x4F` is white on red.
In this mode bit 7 means blink by default, so backgrounds are limited to the 8 dark colours.

Writing a cell changes the screen at once, because the video card reads this memory to draw
every frame. The blinking hardware cursor is separate. It is a cell index held in the CRT
controller (CRTC), reached through an index port, `0x3D4`, and a data port, `0x3D5`. Register
`0x0E` holds the high byte of the index and `0x0F` the low byte.

There is one more detail. The BIOS has already printed the boot messages, and stage 2 should
continue below them, not on top of them. The BIOS keeps the cursor position of text page 0 in
its data area at `0x450`: first the column, then the row. Real mode is over, but that memory is
still there, so it can still be read.

`project/ch03/boot/print32.inc` (part)
```nasm
VGA_TEXT    equ 0xB8000
VGA_COLS    equ 80
VGA_ROWS    equ 25
BDA_CURSOR  equ 0x0450          ; BIOS data area: page 0 cursor (column, row)
VGA_CRTC    equ 0x3D4           ; CRT controller: index port, data at +1

bits 32

; ----------------------------------------------------------------------------
; pm_con_init — continue the screen where the BIOS left its cursor.
;   Clobbers: eax
; ----------------------------------------------------------------------------
pm_con_init:
    movzx eax, byte [BDA_CURSOR]
    mov [con_col], eax
    movzx eax, byte [BDA_CURSOR + 1]
    mov [con_row], eax
    ret

; ----------------------------------------------------------------------------
; pm_putc — one character to the screen (colour con_attr) and to COM1.
;   In:       al = character
;   Clobbers: nothing
; ----------------------------------------------------------------------------
pm_putc:
    pushad
    mov bl, al
    cmp bl, 10
    jne .serial
    mov al, 13                  ; "\n" -> "\r\n" on the serial line
    call serial_out
.serial:
    mov al, bl
    call serial_out

    cmp bl, 10
    je .newline
    mov eax, [con_row]
    imul eax, VGA_COLS
    add eax, [con_col]          ; eax = cell index
    mov bh, [con_attr]
    mov [VGA_TEXT + eax * 2], bx ; character in the low byte, colour high
    inc dword [con_col]
    cmp dword [con_col], VGA_COLS
    jb .cursor
.newline:
    mov dword [con_col], 0
    inc dword [con_row]
    cmp dword [con_row], VGA_ROWS
    jb .cursor
    ; Off the bottom: move rows 1-24 up one row, blank row 24.
    mov esi, VGA_TEXT + VGA_COLS * 2
    mov edi, VGA_TEXT
    mov ecx, VGA_COLS * (VGA_ROWS - 1) / 2  ; dwords: two cells each
    rep movsd
    mov eax, 0x07200720         ; two blank cells, grey on black
    mov ecx, VGA_COLS / 2
    rep stosd
    mov dword [con_row], VGA_ROWS - 1
.cursor:
    call vga_move_cursor
    popad
    ret

; vga_move_cursor — put the blinking hardware cursor at con_row/con_col.
; CRTC registers 0x0E/0x0F hold the cursor's cell index, high byte first.
; Clobbers eax, ecx, edx.
vga_move_cursor:
    mov ecx, [con_row]
    imul ecx, VGA_COLS
    add ecx, [con_col]
    mov dx, VGA_CRTC
    mov al, 0x0E
    out dx, al
    inc dx
    mov al, ch
    out dx, al
    dec dx
    mov al, 0x0F
    out dx, al
    inc dx
    mov al, cl
    out dx, al
    ret
```

- `mov [VGA_TEXT + eax * 2], bx` uses a 32-bit addressing mode with a scaled index, which real
  mode cannot do. It is also the first use of memory above `0xFFFF` without a segment trick.
  BL holds the character and BH the colour, and little-endian order puts BL first.
- Strings in 32-bit code end lines with `\n` alone, like C. The screen needs only "go to the
  next row". A serial terminal needs CR to return to column 0, so `pm_putc` sends `\r\n` on
  COM1. `make test` compares COM1 output line by line, so the line endings must stay the same
  as in real mode.
- Scrolling moves rows 1–24 up one row: 24 × 80 cells is 960 dwords for `rep movsd`. Then it
  fills the last row with `0x0720` (a space, grey on black). `cld` in `pm_entry` is what makes
  `rep` go forwards.
- `serial_out` (not shown) is Chapter 1's polling loop in 32-bit form, using `COM1`,
  `COM1_LSR` and `LSR_THRE` from `print16.inc`: wait for "transmit holding register empty",
  then `out` the byte. Port I/O works the same in both modes.

The rest of the file is short. `pm_print` loops `lodsb`/`pm_putc` until it reaches a zero
byte. `PM_PRINT` is the 32-bit twin of Chapter 2's `PRINT` macro: it puts the string inline and
jumps over it. `pm_print_hex` prints EAX as `0x` and eight digits by rotating one nibble at a
time into the low bits. The variables `con_row`, `con_col` and `con_attr` (`0x0A`, light green
on black) sit at the bottom of the file, inside stage 2's 8 KiB.

## Proof: printing from protected mode

The rest of `pm_entry` prints three facts that only hold in protected mode:

`project/ch03/boot/stage2.asm` (part)
```nasm
    call pm_con_init
    PM_PRINT `sprout: hello from 32-bit protected mode\n`
    PM_PRINT `sprout: CR0 = `
    mov eax, cr0
    call pm_print_hex
    PM_PRINT `, CS = `
    mov eax, cs
    call pm_print_hex
    PM_PRINT `, DS = `
    mov eax, ds
    call pm_print_hex
    PM_PRINT `\n`

    ; Read a byte above 1 MiB: impossible in real mode without tricks.
    mov dword [0x100000], 0x12345678
    PM_PRINT `sprout: wrote and read back 0x100000: `
    mov eax, [0x100000]
    call pm_print_hex
    PM_PRINT `\n`
    PM_PRINT `sprout: stage 2 done, halting\n`
.halt:
    cli
    hlt
    jmp .halt
```

`mov eax, cs` zero-extends the selector, so the printout shows `0x00000008`. Here is the real
run, with the E820 table lines from Chapter 2 removed:

```console
$ make test
sh tools/qemu-test.sh "qemu-system-i386 -m 128M" build/sprout.img tests/boot.expected
PASS: build/sprout.img (18 lines of serial output match tests/boot.expected)
$ make demo
sprout: boot sector, loading stage 2
sprout: stage 2 loaded, jumping to it
sprout: stage 2 running at 0x8000
sprout: E820 memory map (6 entries)
...
sprout: usable memory: 130559 KiB
sprout: A20 line already enabled
sprout: entering protected mode
sprout: hello from 32-bit protected mode
sprout: CR0 = 0x00000011, CS = 0x00000008, DS = 0x00000010
sprout: wrote and read back 0x100000: 0x12345678
sprout: stage 2 done, halting
```

- **CR0 = `0x11`** is bit 0 (PE, which we set) plus bit 4 (ET, "extension type"). ET reports a
  387-style coprocessor interface, and the CPU (here QEMU) keeps it set. Exercise 3 shows CR0
  before the switch.
- **CS = 8, DS = `0x10`**: the selectors for GDT entries 1 and 2.
- **`0x12345678` at 1 MiB.** The write and the read used a plain 32-bit address. With A20
  closed on a real machine, the write would have landed at address 0, on top of the interrupt
  vector table.
- `tests/boot.expected` grew from 13 to 18 lines, and the harness compares all of them.

In the window (`make run`), the BIOS and real-mode lines stay grey. Every line printed after
the switch is light green, written straight to `0xB8000` and continuing where the BIOS cursor
was:

![sprout in QEMU: grey lines from the BIOS and real-mode stage 2, then green lines written
directly to VGA memory in protected mode](images/ch03-protected-mode.png)

## When it goes wrong: the triple fault

Mistakes in protected mode do not print error messages. When the CPU hits a fault (a bad
selector, a limit violation, an invalid opcode), it looks up a handler in the IDT. If that
fails too, it raises a *double fault* (#DF, vector 8). If delivering the double fault also
fails, the CPU gives up and does a **triple fault**: it resets itself. On a real PC the screen
goes black and the BIOS starts again, and you see the machine reboot in a loop with no clue
why.

This example makes one mistake on purpose:

`examples/ch03/triple.asm`
```nasm
; triple.asm — a protected-mode mistake and what it looks like: a triple
; fault. We load DS with selector 0x18, but our GDT has only 3 entries
; (0x00, 0x08, 0x10). The CPU raises a general protection fault (#GP,
; vector 13); there is no IDT to handle it, which is a double fault
; (#DF, vector 8); that can't be handled either: a triple fault, and the
; CPU resets. Run with:  make serial NAME=triple QEMU_D="-d int,cpu_reset"
bits 16
org 0x7C00

start:
    jmp 0:.go
.go:
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov sp, 0x7C00
    mov si, msg
    call puts

    cli
    lgdt [gdt_ptr]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pm

bits 32
pm:
    mov ax, 0x18                ; BUG: one past the end of the GDT
    mov ds, ax                  ; -> #GP -> #DF -> triple fault
    hlt

bits 16
%include "com1.inc"

msg: db "switching to protected mode with a bad selector...", 13, 10, 0

    align 8
gdt:
    dq 0
    dq 0x00CF9A000000FFFF       ; 0x08: code, base 0, limit 4 GiB
    dq 0x00CF92000000FFFF       ; 0x10: data
gdt_ptr:
    dw $ - gdt - 1
    dd gdt

    times 510 - ($ - $$) db 0
    dw 0xAA55
```

```console
$ make serial NAME=triple
timeout 3 qemu-system-i386  -drive format=raw,file=build/triple.img -no-reboot \
    -display none -serial stdio -monitor none
switching to protected mode with a bad selector...
```

There is one line, and then QEMU exits by itself well before the 3 seconds are up (no `timeout`
message this time). The `-no-reboot` flag in the Makefile turns "the guest reset the machine"
into "QEMU exits". Without it, QEMU would reset the virtual machine like real hardware: SeaBIOS
would start again, boot the same disk, and fault again.

QEMU can show what happened. `-d int` logs every interrupt and exception with a full register
dump, and `-d cpu_reset` logs resets. The log is 519 lines on this machine, mostly SeaBIOS's own
interrupts. This excerpt starts at our message, with most register lines of each dump removed
and the interesting ones kept:

```console
$ make serial NAME=triple QEMU_D="-d int,cpu_reset"
...
switching to protected mode with a bad selector...
check_exception old: 0xffffffff new 0xd
     0: v=0d e=0018 i=0 cpl=0 IP=0008:00007c2d pc=00007c2d SP=0000:00007c00 env->regs[R_EAX]=00000018
EAX=00000018 EBX=00000000 ECX=00000000 EDX=00000080
EIP=00007c2d EFL=00000006 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
DS =0000 00000000 0000ffff 00009300 DPL=0 DS16 [-WA]
GDT=     00007cc0 00000017
IDT=     00000000 000003ff
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
check_exception old: 0xd new 0xd
     1: v=08 e=0000 i=0 cpl=0 IP=0008:00007c2d pc=00007c2d SP=0000:00007c00 env->regs[R_EAX]=00000018
...
check_exception old: 0x8 new 0xd
Triple fault
```

The log tells the whole story:

- `v=0d e=0018`: exception vector 13 (`0x0d`), general protection (#GP), with error code
  `0x18`. For a bad selector, the error code *is* the selector. `IP=0008:00007c2d` is the
  faulting instruction: `mov ax, 0x18` at `0x7C29` is 4 bytes in 32-bit code, so `0x7C2D` is
  the `mov ds, ax`.
- `CS =0008 ... CS32`: the far jump worked, and we are in 32-bit code. `DS =0000 ... DS16`: DS
  still holds its real-mode cache, because the load that would have replaced it failed.
- `GDT= 00007cc0 00000017`: our table at `0x7CC0`, limit 23, so 3 entries. Selector `0x18`
  needs bytes `0x18..0x1F`, past the limit.
- `IDT= 00000000 000003ff`: the real-mode vector table. To deliver #GP the CPU reads the 8-byte
  gate for vector 13 from `0x68`. Those bytes are two real-mode far pointers, not a valid
  protected-mode gate, so delivery faults again (`old: 0xd new 0xd`). Two faults in a row
  become a double fault: `v=08`.
- Delivering #DF reads gate 8 from the same table and fails the same way (`old: 0x8 new 0xd`).
  A fault while delivering a double fault is a `Triple fault`: the CPU shuts down, and the PC
  resets.

The same thing happens if you forget `cli` and a timer interrupt arrives, or if a later chapter
loads a broken IDT. `-d int,cpu_reset -no-reboot` is the first tool to reach for when a kernel
"just reboots". Chapter 10 adds gdb.

### Common mistakes

| mistake | what you see |
|---|---|
| no far jump after `mov cr0, eax` | 32-bit code runs as 16-bit code; output stops (exercise 2) |
| no `cli` before the switch | a triple fault at the next timer tick |
| no `bits 32` before `pm_entry` | 16-bit code run as 32-bit: the same mess, reversed |
| GDT limit = size, not size − 1 | usually harmless: one byte too many is "valid" |
| selector `1` instead of `8` | `1` is index 0 with RPL 1, the null selector: loading CS faults |
| setting bit 0 of port `0x92` | the machine resets |
| data segment registers not reloaded | real-mode 64 KiB limits: #GP above 64 KiB on a real CPU |

## History: the 286 that couldn't go back

Protected mode first appeared in Intel's 80286, introduced on 1 February 1982. It had 24-bit
physical addresses (16 MiB), descriptors, privilege levels, and no way back. Once PE was set,
the only way to return to real mode was to reset the CPU. That was a real problem in a world of
DOS programs and BIOS services that only worked in real mode.

IBM's PC/AT (1984) worked around it in hardware and firmware. Software that wanted to get back
to real mode wrote a "shutdown" code into the CMOS RAM of the real-time clock, stored a resume
address at `0040:0067` in the BIOS data area, and reset the CPU. Instead of booting, the BIOS
checked the shutdown byte at the reset vector and jumped to the saved address, back in real
mode with memory intact. There were two ways to cause the reset:

- **The keyboard controller.** Writing `0xFE` to port `0x64` makes the 8042 pulse the CPU's
  reset line. The 8042 was already involved in memory management, since it also held the A20
  gate.
- **A triple fault, on purpose.** Load an IDT with limit 0 and cause an interrupt. Every
  delivery fails, which is the same chain our triple-fault example ran into by accident. The
  CPU shuts down, and the AT turns the shutdown into a reset. This was faster than waiting for
  the slow keyboard controller.

Both were used in production. Windows 3.1's standard mode used the faster triple-fault method.
IBM's OS/2 1.0 and 1.1 used the triple-fault method on the 286. The 80386 (1985) finally made
the way back easy: clear PE and reload the segments.

The A20 gate lived on even longer. DOS used it as a feature: with the gate open, real-mode
`FFFF:0010`–`FFFF:FFFF` reaches the first 64 KiB above 1 MiB (minus 16 bytes). This is the "high
memory area" that `HIMEM.SYS` managed and that `DOS=HIGH` loaded DOS into. From the 486 on,
the CPU itself has an `A20M#` input pin for the masking. Intel's manuals now say `A20M#` may be
absent on newer processors (from Haswell on, according to Wikipedia's A20 article). Boot
loaders still check the line, and xv6's boot sector opens it through the 8042 exactly as
described above.

## Summary

- Real mode reaches 1 MiB in 64 KiB segments, with no protection. 32-bit protected mode reaches
  4 GiB, and segment registers hold selectors into a descriptor table. The price is the BIOS,
  which cannot be called any more.
- A20 must be open, or bit 20 of every address is forced to 0. Test it by comparing
  `0000:0500` with `FFFF:0510`. Open it with the BIOS (`int 0x15`, `AX=0x2401`), the 8042, or
  port `0x92` bit 1, and never touch bit 0. QEMU 6.2 does not apply a closed gate while paging
  is off, but the test is still right for hardware.
- A GDT descriptor is 8 bytes with base, limit, access byte (`0x9A` code, `0x92` data) and flags
  (`0xC`: 4 KiB granularity, 32-bit). sprout uses the flat model: base 0, limit 4 GiB, so an
  address is an address. `lgdt` takes a 6-byte operand: size − 1, then the linear base.
- The switch: `cli`, `lgdt`, set CR0.PE, far jump to a 32-bit code selector, then reload every
  data segment register and the stack. Only the far jump refreshes CS's descriptor cache.
- Output in protected mode means writing to the VGA buffer at `0xB8000` (character byte +
  attribute byte, cell = `row * 80 + col`), moving the cursor through CRTC registers
  `0x0E`/`0x0F`, and polling COM1 as before.
- A fault with no usable IDT becomes a double fault and then a triple fault, which resets the
  machine. `qemu-system-i386 -d int,cpu_reset -no-reboot` shows the chain.

## Exercises

1. **A segment for the screen.** Add a fourth GDT entry with base `0xB8000`, limit 3999, byte
   granularity, and access byte `0x92`. In `pm_entry`, load its selector into FS and put a
   white-on-red `!` (`0x4F21`) in the bottom-right corner with a write to `[fs:3998]`. Then
   write one cell *past* the limit, `[fs:4000]`. What should happen, and what does QEMU do?

<details><summary>Solution</summary>

`boot/gdt.inc`: one selector and one entry. Flags `0x4` means D=1 with G=0, so the limit counts
bytes and the segment ends at `0xB8000 + 3999`.

```nasm
TEXT_SEL    equ gdt.text - gdt      ; 0x18
...
.data:  GDT_ENTRY 0, 0xFFFFF, 0x92, 0xC     ; ring 0 data: read/write
.text:  GDT_ENTRY 0xB8000, 3999, 0x92, 0x4   ; the text screen, bytes
.end:
```

`boot/stage2.asm`, after the `0x100000` test:

```nasm
    mov ax, TEXT_SEL
    mov fs, ax
    mov word [fs:4000], 0x4F21  ; one cell past the limit: #GP on a real CPU
    PM_PRINT `sprout: wrote to [fs:4000] and lived\n`
    PM_PRINT `sprout: stage 2 done, halting\n`
    mov word [fs:3998], 0x4F21  ; '!', white on red, in the last cell
```

The `!` must be written *after* the last print. Printing the last line scrolls the screen, and
the scroll would move an earlier `!` up a row. A first attempt that wrote it before the prints
ended up three rows above the corner.

The QEMU monitor can show the segment. `info registers` prints the hidden descriptor cache:

```console
$ (sleep 2; printf 'info registers\nquit\n') | timeout 10 qemu-system-i386 -m 128M \
    -drive format=raw,file=build/sprout.img -no-reboot -display none \
    -serial file:build/serial.log -monitor stdio | grep -E '^(FS |GDT)'
FS =0018 000b8000 00000f9f 00409300 DPL=0 DS   [-WA]
GDT=     00008448 0000001f
$ tr -d '\r' < build/serial.log | tail -2
sprout: wrote to [fs:4000] and lived
sprout: stage 2 done, halting
```

Base `0xB8000`, limit `0xF9F` = 3999, and the GDT limit is now 31 (4 entries). `make run` shows
the red `!` in the corner.

The write to `[fs:4000]` should raise #GP, because offset 4000 + 1 is beyond the limit. With no
IDT, that ends in a triple fault. QEMU 6.2 does not check segment limits on data accesses, so
the program "lived". Writing one cell past the screen is harmless here (`0xB8FA0` is unused
video memory). On a real CPU, the limit check is the whole point of the segment. This is one
more reason the course uses flat segments and leaves protection to paging, which QEMU does
enforce.

</details>

2. **Forget the far jump.** Replace `jmp CODE_SEL:pm_entry` with a near `jmp pm_entry`. Build,
   run `make test`, and use the QEMU monitor to find out where the CPU is.

<details><summary>Solution</summary>

`ndisasm` shows the change: the 5-byte far jump `EA8B850800` is now a 3-byte near jump.

```console
$ ndisasm -b 16 -o 0x8000 build/stage2.bin | grep 0000818F
0000818F  E9F903            jmp 0x858b
$ make test
sh tools/qemu-test.sh "qemu-system-i386 -m 128M" build/sprout.img tests/boot.expected
--- tests/boot.expected	2026-11-18 22:06:06.555802252 -0500
+++ build/serial.log	2026-11-18 22:18:08.739976904 -0500
@@ -12,7 +12,3 @@
 sprout: usable memory: 130559 KiB
 sprout: A20 line already enabled
 sprout: entering protected mode
-sprout: hello from 32-bit protected mode
-sprout: CR0 = 0x00000011, CS = 0x00000008, DS = 0x00000010
-sprout: wrote and read back 0x100000: 0x12345678
-sprout: stage 2 done, halting
FAIL: serial output differs from tests/boot.expected (see build/serial.log)
make: *** [Makefile:48: test] Error 1
```

Every line after `sprout: entering protected mode` is missing. The monitor shows why:

```console
$ (sleep 2; printf 'info registers\nquit\n') | timeout 10 qemu-system-i386 -m 128M \
    -drive format=raw,file=build/sprout.img -no-reboot -display none \
    -serial file:build/serial.log -monitor stdio | grep -E '^(EIP|CS |CR0)'
EIP=00000001 EFL=00000002 [-------] CPL=0 II=0 A20=1 SMM=0 HLT=0
CS =0000 00000000 0000ffff 00009b00 DPL=0 CS16 [-RA]
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
```

CR0 says protected mode, but CS is still `0000` with a 16-bit cache (`CS16`). The CPU executed
`pm_entry`'s bytes as 16-bit code (`mov eax,0xd88e0010`, `mov es,ax`, ... as shown in the
chapter), wandered off, and was at EIP 1 when the monitor looked. Nothing ever reached
`pm_print`. It did not fault either. With the real-mode table as IDT, any fault would have
become a triple fault, and with `-no-reboot` QEMU would have exited before the monitor could
answer.

</details>

3. **CR0 before the switch.** `mov eax, cr0` works in real mode too. Print CR0 just before
   `sprout: entering protected mode`, using the real-mode helpers from `print16.inc`. Which bits
   are set?

<details><summary>Solution</summary>

```nasm
    PRINT `sprout: CR0 in real mode = `
    mov eax, cr0
    call print_hex32
    call print_crlf
    PRINT `sprout: entering protected mode\r\n`
```

```console
$ make demo | grep CR0
sprout: CR0 in real mode = 0x00000010
sprout: CR0 = 0x00000011, CS = 0x00000008, DS = 0x00000010
```

Only ET (bit 4) is set. The switch added PE (bit 0), and nothing else changed. Paging (bit 31)
stays off until Chapter 7. `make test` now fails, because `tests/boot.expected` has no line for
CR0. Add the line there if you keep the change.

</details>

4. **Colours.** Make protected-mode text white on blue. Then add a `PM_ERROR "text"` macro that
   prints one string in white on red and restores the normal colour. Use it to report a failed
   memory test after the `0x100000` read-back.

<details><summary>Solution</summary>

`boot/print32.inc`: change the attribute and add the macro after `PM_PRINT`.

```nasm
; PM_ERROR "text" — print in white on red, then restore the colour.
%macro PM_ERROR 1
    push eax
    mov al, [con_attr]
    mov byte [con_attr], 0x4F
    PM_PRINT %1
    mov [con_attr], al
    pop eax
%endmacro
...
con_attr:   db 0x1F             ; white on blue
```

`boot/stage2.asm`, after the read-back. EAX still holds the value read, because `PM_PRINT`
preserves it.

```nasm
    PM_PRINT `\n`
    cmp eax, 0x12345678
    je .mem_ok
    PM_ERROR `sprout: memory above 1 MiB does not work\n`
.mem_ok:
```

`make test` still passes with 18 lines, because the error line is not printed on a working
machine. To see it, change the `cmp` to `0x12345679` for one run: `make run` shows a red line
between the blue ones. COM1 carries no colours, so the serial output only gains the text line.

</details>

Sources for this chapter: [REFERENCES.md](REFERENCES.md#chapter-3--protected-mode)
