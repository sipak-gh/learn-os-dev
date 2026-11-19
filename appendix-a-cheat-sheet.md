# Appendix A — Cheat Sheet

The numbers, ports, bit layouts and commands of the course on a few pages: the build and QEMU
commands, sprout's disk and memory layout, the BIOS calls of the loader, the protected-mode
structures (GDT, IDT, TSS, page tables), the PC devices sprout drives, the system calls, the gdb
and QEMU debugging commands, the error messages you will meet, and a map from each idea to the
file that implements it. Values are those of `project/ch10/` unless a chapter is named. For the
reasoning behind each entry, read the chapter; this page is for looking things up.

Previous: [Chapter 10](10-debugging-and-next-steps.md). Next: [REFERENCES.md](REFERENCES.md).

## Tools and commands

Install (Ubuntu or Debian): `sudo apt install build-essential nasm qemu-system-x86 gdb`. The
course was verified with NASM 3.02, gcc 11.4, GNU ld 2.38, QEMU 6.2 (SeaBIOS 1.15) and gdb 12.1.

**make targets** (run in `project/chNN/`):

| target | what it does |
|---|---|
| `make` | build `build/sprout.img`, a 1 MiB raw disk image |
| `make run` | boot in a QEMU window; COM1 on this terminal |
| `make run-nox` | VGA text in the terminal (`-display curses`); quit: Esc then 2 for the monitor, `q` |
| `make test` | (Ch. 4+: unit tests on the host, then) a headless boot, COM1 diffed with `tests/boot.expected` |
| `make unit` | (Ch. 4+) only the host unit tests of `lib/`; `build/unit -v`, `build/unit NAME` |
| `make demo` | headless boot, print the COM1 transcript |
| `make disasm` | `ndisasm` the boot sector and stage 2; from Ch. 4 also `objdump` the kernel |
| `make debug` | (Ch. 10) QEMU with `-s -S`: stopped at the reset vector, gdb server on localhost:1234 |
| `make gdb` | (Ch. 10, second terminal) `gdb -q -x tools/gdbinit` |
| `make clean` | remove `build/` |
| `QEMUEXTRA="..."` | (Ch. 10) extra QEMU options for `run`, `run-nox` and `debug` |

Examples (`examples/chNN/`): Chapters 1–3 build images with `make` and boot one headless with
`make serial NAME=x` (Chapters 1 and 3 also have `make run NAME=x`; Chapter 3 adds
`QEMU_D="-d ..."`); from Chapter 4 on, each example is its own target (`make divzero`,
`make nosave`, `make crash`, ...).

**Building by hand:**

```console
$ nasm -f bin -w+all -w-reloc -l build/boot.lst -o build/boot.bin boot/boot.asm
$ nasm -f bin -w+all -w-reloc -I boot/ -MD build/stage2.d -MP -o build/stage2.bin boot/stage2.asm
$ cat build/boot.bin build/stage2.bin build/kernel.bin > build/sprout.img
$ truncate -s 1M build/sprout.img
$ xxd -s 496 build/boot.bin                     # must end in 55aa
$ ndisasm -b 16 -o 0x7c00 build/boot.bin | head
```

- `-w-reloc` silences NASM's "section-crossing relocation" warnings, which every absolute
  address in a flat binary triggers. `-MD file -MP` writes make dependencies for `%include`s.
- From Chapter 4, stage 2 is assembled after the kernel with
  `-DKERNEL_SECTORS=$(( (size + 511) / 512 ))`.

**Kernel compiler and linker flags** (Chapter 4; Chapter 10 adds `-fno-omit-frame-pointer`):

| flag | why |
|---|---|
| `-m32` | i386 code from the 64-bit host gcc |
| `-ffreestanding` | no hosted C library; `__STDC_HOSTED__` is 0, implies `-fno-builtin` |
| `-fno-pie` | absolute addresses, no `__x86.get_pc_thunk` and GOT |
| `-fno-stack-protector` | no canary read from `gs:0x14`, no `__stack_chk_fail_local` |
| `-fcf-protection=none` | no `endbr32` |
| `-fno-asynchronous-unwind-tables` | no `.eh_frame` |
| `-mgeneral-regs-only` | no x87/SSE: a `double` would need `__muldf3` |
| `-fno-omit-frame-pointer` | (Ch. 10) an `ebp` chain for backtraces; `-O2` omits it |
| `-std=c11 -O2 -g -Wall -Wextra -Werror` | as in learn-c |
| `ld -m elf_i386 -nostdlib -T kernel/linker.ld` | 32-bit ELF, no libraries, our layout; then `objcopy -O binary` |

No `-lgcc`: 64-bit `/` and `%` by a variable need `__udivdi3`, so `lib/fmt.c` divides with
`fmt_divmod64` (32-bit `div` only). gcc may still emit calls to `memcpy`, `memmove`, `memset`
and `memcmp`; `lib/string.c` defines them.

## QEMU

| option | meaning |
|---|---|
| `-m 128M` | RAM size; the E820 map in `tests/boot.expected` depends on it |
| `-drive format=raw,file=build/sprout.img` | the image as the first IDE hard disk, BIOS drive 0x80 |
| `-no-reboot` | a triple fault (or reset) ends QEMU instead of rebooting forever |
| `-serial stdio` / `-serial file:F` / `-serial null` | COM1 on the terminal / into a file / nowhere |
| `-display none` / `-display curses` | no window / VGA text in the terminal |
| `-monitor stdio` / `-monitor none` | the QEMU monitor on the terminal / off |
| `-S` | create the machine, don't start the CPU |
| `-s` | gdb server on localhost:1234 (with `-S`: stopped at the reset vector) |
| `-d int,cpu_reset -D F` | log every interrupt, exception and CPU reset to file F |
| `-d mmu` | MMU events, among them `A20 update` (Ch. 3) |
| `-debugcon file:F -global isa-debugcon.iobase=0x402` | SeaBIOS's debug log (it writes to port 0x402) |
| `-device isa-debug-exit,iobase=0xf4,iosize=0x04` | (Ch. 10 exercise) `outb(0xf4, v)` ends QEMU with status `(v << 1) \| 1` |

Every non-interactive run goes under `timeout`: a halted guest never exits by itself
(`timeout` exits with 124).

**Monitor commands:** `info registers` (CPU state, CR0–CR4, GDT, IDT), `info pic` (pic0 is the
master), `info mem` (mapped ranges), `info tlb` (every mapping), `xp /4xb ADDR`, `xp /3wx 0x500`,
`xp /2i 0xfffffff0` (physical memory as bytes, words or instructions), `sendkey a`,
`sendkey shift-a`, `sendkey ret`, `sendkey up`, `screendump shot.ppm`, `q`.

**`-d int` log lines:** `N: v=VV e=EEEE i=0 cpl=C IP=CS:EIP ... CR2=...` (one per event; `v=` is
the vector in hex, `e=` the error code), `check_exception old: X new Y` (a fault while
delivering a fault), `Triple fault`. Read the *first* fault of the chain: `v=0d` or `v=0e`, then
`v=08`, then the reset.

## Disk image and memory layout

| LBA (512-byte sectors) | content |
|---|---|
| 0 | boot sector `boot/boot.asm` |
| 1–16 | stage 2 `boot/stage2.asm`, padded to exactly 8 KiB (`STAGE2_SECTORS`) |
| 17– | kernel flat binary (`KERNEL_LBA`), Ch. 4+ |
| to 2048 | zero padding to 1 MiB (`truncate -s 1M`) |

| physical address | what |
|---|---|
| 0x00000–0x004FF | real-mode IVT (0x000–0x3FF) and BIOS data area; BDA 0x450 = cursor column, row |
| 0x00500 | `BOOT_INFO`: `boot_drive`, `e820_count`, `e820_map` (three dwords) |
| 0x00600–0x008FF | E820 map, at most 32 × 24-byte entries |
| 0x07C00 | boot sector; the real-mode stack grows down from here |
| 0x08000 | stage 2 |
| 0x10000 | kernel as loaded from disk, 64 sectors per read; at most 1144 sectors (to 0x9F000) |
| 0x9FC00 | EBDA (QEMU, 128 MiB) |
| 0xB8000 | VGA text buffer |
| 0xF0000 | BIOS ROM (and 0xFFFC0000: the BIOS flash below 4 GiB) |
| 0x100000 | kernel, where it is linked and runs (`_start` in `.text.entry`) |
| 0x200000– | frames handed out by the frame allocator (`PMM_FIRST`), up to 1 GiB (`PMM_LIMIT`) |

| virtual address (Ch. 7+) | what |
|---|---|
| 0x00000000 | page 0, unmapped: NULL dereferences fault |
| 0x00001000–top of RAM | identity-mapped RAM; the kernel's `.text` and `.rodata` read-only |
| 0x40000000–0x7FFFFFFF | user space (`USER_BASE`–`USER_TOP`, Ch. 9); programs linked at 0x40000000 |
| below 0x80000000 | the user stack, 16 KiB (`USER_STACK_SIZE`) |
| 0xD0000000 | kernel heap (`KHEAP_START`), at most 64 MiB |

**QEMU's E820 map with `-m 128M`:**

| base | end | type |
|---|---|---|
| 0x00000000 | 0x0009FBFF | 1 usable (639 KiB) |
| 0x0009FC00 | 0x0009FFFF | 2 reserved (EBDA) |
| 0x000F0000 | 0x000FFFFF | 2 reserved (BIOS) |
| 0x00100000 | 0x07FDFFFF | 1 usable |
| 0x07FE0000 | 0x07FFFFFF | 2 reserved |
| 0xFFFC0000 | 0xFFFFFFFF | 2 reserved (BIOS flash) |

130559 KiB usable. With 4 GiB or more, RAM above 3 GiB moves above 4 GiB (the PCI hole).

**Hand-off from stage 2 to the kernel (Ch. 4+):** jump to 0x100000 in protected mode with
EAX = `BOOT_MAGIC` 0x54525053 ("SPRT" in memory) and EBX = 0x500 (`BOOT_INFO`), like
Multiboot's magic in EAX and info pointer in EBX. `kmain(magic, info)` panics on a wrong magic.

## Real mode and the BIOS (Chapters 1–2)

- **Reset:** a 386 or later starts with CS = F000 (base 0xFFFF0000), EIP = 0xFFF0: the first
  fetch is at 0xFFFFFFF0. The 8086 started at F000:FFF0 = 0xFFFF0.
- **Boot contract:** the BIOS loads sector 0 (512 bytes) to 0x7C00, checks for 0x55 0xAA at
  offsets 510–511, jumps there in real mode with DL = boot drive (0x80 = first hard disk).
  Nothing else is guaranteed. Without the signature SeaBIOS prints
  "Boot failed: not a bootable disk" and tries the next device.
- **0x7C00** = 32 KiB − 1 KiB (0x8000 − 0x400), chosen for the IBM PC 5150 BIOS (1981).
- **Addresses:** physical = segment × 16 + offset; 0000:7C00 = 07C0:0000 = 0x7C00. Zero DS, ES,
  SS with `org 0x7c00`, or far-jump to set CS to match `org`.
- **Boot sector skeleton:** `bits 16`, `org 0x7c00`, code, `times 510 - ($ - $$) db 0`,
  `dw 0xAA55` (stored as 55 AA).
- **32-bit operands** work in real mode on a 386+: NASM adds a 0x66 prefix (0x67 for 32-bit
  addressing). `%%label` is local to one macro expansion.
- **Halt:** `cli` then `hlt`, in a loop (`jmp` back, in case an NMI wakes the CPU).

| BIOS call | in | out |
|---|---|---|
| `int 0x10` AH=0x0E | AL = character, BH = page 0 | teletype output; send 13 then 10 for a new line |
| `int 0x13` AH=0x02 | AL = count, CH = cylinder low 8 bits, CL = sector (bits 0–5) + cylinder bits 8–9 (bits 6–7), DH = head, DL = drive, ES:BX = buffer | CHS read; CF = 1 on error, AH = status |
| `int 0x13` AH=0x08 | DL = drive | DH = last head, CL bits 0–5 = sectors per track (QEMU: 63) |
| `int 0x13` AH=0x41 | BX = 0x55AA, DL = drive | CF clear, BX = 0xAA55, CX bit 0 = AH=0x42 supported |
| `int 0x13` AH=0x42 | DL = drive, DS:SI = disk address packet | LBA read; CF = 1 on error, AH = status |
| `int 0x15` EAX=0xE820 | EDX = 0x534D4150 ("SMAP"), EBX = continuation (0 first), ECX = 24, ES:DI = buffer | EAX = "SMAP", CF clear, ECX = bytes written, EBX = next (0 or CF set: done) |
| `int 0x15` AX=0x2401 | | enable A20 |

- **CHS from LBA:** `sector = LBA % spt + 1`, `head = (LBA / spt) % heads`,
  `cylinder = LBA / spt / heads`. Sectors count from 1. int 0x13 CHS tops out at
  1024 × 255 × 63 × 512 bytes ≈ 8 GB; old BIOSes with IDE limits at 504 MiB (1024 × 16 × 63).
- **Disk address packet (16 bytes):** size 0x10, 0, sector count (word), buffer offset (word),
  buffer segment (word), start LBA (qword).
- **int 0x13 status (AH):** 0x01 invalid function or parameter, 0x04 sector not found, 0x09 DMA
  crossed a 64 KiB boundary, 0x20 controller failure, 0x80 timeout.
- **E820:** reload EAX and EDX before every call; skip zero-length entries. Entry: base
  (qword), length (qword), type (dword), ACPI 3.0 attributes (dword). Types: 1 usable,
  2 reserved, 3 ACPI reclaimable, 4 ACPI NVS, 5 bad. Only type 1 is RAM; gaps are not RAM.

## COM1 (16550 UART at 0x3F8)

| port | register (DLAB = 0) | with DLAB = 1 |
|---|---|---|
| 0x3F8 | transmit / receive data | divisor, low byte |
| 0x3F9 | interrupt enable (IER; bit 0 = received data, Ch. 6) | divisor, high byte |
| 0x3FA | FIFO control (FCR, write) | |
| 0x3FB | line control (LCR; bit 7 = DLAB) | |
| 0x3FC | modem control (MCR) | |
| 0x3FD | line status (LSR; bit 0 = byte received, bit 5 = THRE, transmitter empty) | |

- **Init:** IER = 0; LCR = 0x80 (DLAB); divisor 3 (115200 / 3 = 38400 baud); LCR = 0x03 (8N1).
  Chapter 4 adds FCR = 0xC7 (FIFOs on and cleared, 14-byte threshold); Chapter 6 uses FCR = 0x07
  (trigger level 1), IER = 0x01 and MCR = 0x0B (DTR, RTS, OUT2; OUT2 gates the IRQ on real PCs).
- **Send:** wait for LSR bit 5, then `out 0x3F8`. **Receive (Ch. 6):** IRQ 4; read while LSR bit
  0 is set; CR → `'\n'`, DEL (0x7F) → `'\b'`.

## Protected mode (Chapter 3)

**A20:** test by comparing `0000:0500` with `FFFF:0510` (the same byte if A20 wraps; ZF = 1
means off). Enable with `int 0x15` AX=0x2401, "fast A20" (port 0x92 bit 1; never set bit 0, it
resets the CPU), or the 8042 (0xD1 to port 0x64, then 0xDF to port 0x60). Re-check after each.
QEMU 6.2 doesn't apply a closed A20 gate while paging is off.

**GDT descriptor** (8 bytes): base (32 bits) and limit (20 bits) split across the entry, access
byte, flags nibble.

| selector | descriptor | access | flags | meaning |
|---|---|---|---|---|
| 0x00 | `dq 0` | | | null |
| 0x08 | `dq 0x00CF9A000000FFFF` | 0x9A | 0xC | kernel code, base 0, limit 0xFFFFF × 4 KiB = 4 GiB, 32-bit |
| 0x10 | `dq 0x00CF92000000FFFF` | 0x92 | 0xC | kernel data, same range |
| 0x1B | (Ch. 9) | 0xFA | 0xC | user code: 0x18 \| RPL 3 |
| 0x23 | (Ch. 9) | 0xF2 | 0xC | user data: 0x20 \| RPL 3 |
| 0x28 | (Ch. 9) | 0x89 | | TSS, limit `sizeof tss - 1` |

- Access byte: present (bit 7), DPL (bits 6–5), code/data (bit 4), type (bits 3–0). Flags 0xC:
  4 KiB granularity, 32-bit. A selector is index × 8 plus TI (bit 2) and RPL (bits 1–0).
- `lgdt` / `lidt` take 6 bytes: a 16-bit limit (size − 1), then the 32-bit linear base.
- **The switch:** `cli`; `lgdt [gdt_ptr]`; `mov eax, cr0` / `or eax, 1` / `mov cr0, eax`;
  `jmp CODE_SEL:pm_entry` (the far jump is the only way to reload CS's descriptor cache); then
  `bits 32`, load DS, ES, FS, GS, SS with 0x10, set ESP, `cld`. CR0 is then 0x11 (PE + ET).
  Interrupts stay off until there is an IDT (Ch. 5): the IDTR still points at the real-mode IVT.

**VGA text mode:** 80 × 25 cells at 0xB8000, cell `row * 80 + col`, two bytes: character, then
attribute (bit 7 blink, bits 6–4 background, bits 3–0 foreground). 0x07 grey on black, 0x0A light
green, 0x4F white on red. Cursor: CRTC index port 0x3D4, data 0x3D5, registers 0x0E (high byte)
and 0x0F (low byte).

## The C kernel (Chapter 4)

- `kernel/entry.asm` `_start` (section `.text.entry`, first in the linker script, so at
  0x100000): ESP = top of a 16 KiB `.bss` stack, EBP = 0, `cld`, zero `.bss` with `rep stosb`,
  push EBX and the magic, `call kmain`, then `cli; hlt` forever.
- `kernel/linker.ld`: `ENTRY(_start)`, `. = 0x100000`, `.text.entry` first, symbols
  `__kernel_start`, `__bss_start`, `__bss_end`, `__kernel_end` (Ch. 10: `.ksyms` after `.data`,
  `__text_end`), `/DISCARD/` for `.eh_frame` and `.comment`. `objcopy -O binary` writes no bytes
  for `.bss`.
- `kernel/io.h`: `outb` with `"a"(value), "Nd"(port)`; `inb` with `"=a"`; both `volatile`.
  A write to port 0x80 (the POST diagnostic port) is a short I/O delay (`io_wait`, Ch. 5).
- `kprintf` (`lib/fmt.c` + `kernel/kprintf.c`) writes to VGA and COM1; `%llx` and friends via
  `fmt_divmod64`. `panic(...)` = `panic_at(__FILE__, __LINE__, ...)`: prints
  `PANIC at file:line: message` and halts.
- Inspect: `nm -n build/kernel.elf`, `objdump -h`, `objdump -d -M intel build/kernel.elf`.

## Interrupts (Chapter 5)

| vector | name | kind | error code |
|---|---|---|---|
| 0 | #DE divide error | fault | no |
| 3 | #BP breakpoint (`int3`) | trap | no |
| 6 | #UD invalid opcode | fault | no |
| 8 | #DF double fault | abort | yes (0) |
| 13 | #GP general protection | fault | yes |
| 14 | #PF page fault | fault | yes, address in CR2 |
| 32–47 | IRQ 0–15 (`IRQ_BASE` 32; timer 32, keyboard 33, COM1 36) | interrupt | no |
| 0x80 | system call (Ch. 9) | trap gate, DPL 3 | no |

- Vectors 0–31 are reserved for CPU exceptions. Error codes are pushed for 8, 10–14, 17 and 21.
  A fault saves the faulting EIP, a trap or IRQ the next instruction's.
- **IDT gate** (8 bytes): offset low 16 bits, selector 0x08, 0, type, offset high 16 bits. Type
  0x8E = present, DPL 0, 32-bit interrupt gate (clears IF); 0xEF = present, DPL 3, 32-bit trap
  gate (Ch. 9, `int 0x80`). A missing gate, or `int n` from ring 3 on a DPL 0 gate, raises #GP
  with error code n × 8 + 2 (IDT bit set).
- **On entry** the CPU pushes (from ring 3: SS, ESP), EFLAGS, CS, EIP, (error code). The stubs
  (`ISR_NOERR` / `ISR_ERR` in `kernel/isr.asm`) push a dummy 0 where needed and the vector,
  jump to `isr_common`, which saves the registers and calls `interrupt_dispatch(struct regs *)`;
  then `add esp, 8` and `iret`.
- `sti` takes effect after the next instruction, so `sti; hlt` can't miss a wake-up. `hlt`
  with IF = 0 stops the CPU for good.

**8259 PIC:** master 0x20 (command) / 0x21 (data), slave 0xA0 / 0xA1. ICW1 0x11 (init + ICW4),
ICW2 vector base (32, 40), ICW3 master `1 << 2` / slave 2, ICW4 0x01 (8086 mode). EOI:
`outb(0x20, 0x20)`; for IRQ 8–15 to the slave, then the master. Spurious IRQ 7/15: read the ISR
(OCW3 0x0B); bit clear → spurious, no EOI (but a spurious IRQ 15 still needs the master's EOI).

**8254 PIT:** input 1193182 Hz (14.31818 MHz / 12); divisor 16 bits, 0 = 65536 (18.2 Hz, the
BIOS default). sprout: 100 Hz, divisor 11932; command 0x36 to port 0x43 (channel 0, low/high
byte, mode 3), then the divisor low and high byte to port 0x40. Wait with
`while (ticks - start < n) hlt();` (survives wrap-around; `ticks` is `volatile`).

## Keyboard, input and the shell (Chapter 6)

**8042:** data port 0x60, status/command 0x64. Status bit 0 = byte waiting, bit 1 = controller
busy, it has not taken our last byte (wait for clear before writing). Command 0x20 reads, 0x60 writes the configuration byte;
config bit 6 = translate set 2 to set 1. Command 0xFE pulses the CPU reset line (`reboot`).

| key | set 1 make / break |
|---|---|
| `a` | 0x1E / 0x9E |
| Enter | 0x1C / 0x9C |
| Left Shift, Right Shift | 0x2A, 0x36 |
| Left Ctrl | 0x1D |
| Caps Lock | 0x3A |
| Up (extended) | E0 48 / E0 C8 |
| keypad Enter | E0 1C |

Break = make | 0x80. Set 2 (with translation off) sends F0 + make code on release.

- **Ring buffer** (`lib/ringbuf.c`): 256 bytes, free-running `uint32_t` head and tail;
  `count = head - tail`, slot = `i % 256`; full drops. One writer per counter.
- **Lost wake-up:** `input_getc` does `cli`, tries the buffer, and if it is empty runs
  `__asm__ volatile ("sti; hlt")` as one statement; then loops.
- **Line editor** (`lib/line.c`): printable 0x20–0x7E, Backspace echoes `\b \b`, at most 127
  characters. `args_split` cuts in place; `args_parse_u32` takes decimal or 0x hex.
- **Power:** `reboot` = 8042 command 0xFE; `poweroff` = `outw(0x604, 0x2000)` (QEMU ≥ 2.0 only;
  real hardware needs ACPI `\_S5`).
- **Interactive test:** `tools/qemu-test.sh` feeds `tests/boot.in` to COM1 one line per
  `sprout> ` prompt and stops after `poweroff`.

**Shell commands (Ch. 10):**

| command | chapter | does |
|---|---|---|
| `help`, `echo WORDS`, `version`, `mem`, `uptime`, `sleep MS`, `clear`, `crash`, `reboot`, `poweroff` | 6 | the first shell; `mem` = E820 map and (Ch. 7) free frames; `crash` divides by zero |
| `vm [ID]`, `heap`, `alloc BYTES`, `free ADDR`, `crash [div\|null\|code]` | 7 | page mappings (of task ID, Ch. 9), the kernel heap, three ways to crash |
| `ps`, `spawn PROG`, `run PROGS`, `wait ID`, `kill ID` | 8 | tasks; `ps` gets a RING column in Ch. 9 |

Kernel programs: ping, pong, tick, tock, hog (Ch. 8). User programs (Ch. 9, `user/`): hello,
count, snoop, priv, spin.

## Paging and memory (Chapter 7)

- **Linear address:** directory index (bits 31–22) | table index (21–12) | offset (11–0).
  `PDX(va) = va >> 22`, `PTX(va) = (va >> 12) & 0x3FF`. One table maps 4 MiB, a directory 4 GiB.
  Identity-mapping 128 MiB takes 32 page tables.
- **Entry flags:** P 0x001, R/W 0x002, U/S 0x004, A 0x020 (set by the CPU on access), D 0x040
  (set on write), PS 0x080 in a PDE (4 MiB page, with CR4.PSE bit 4). Frame = `entry &
  0xFFFFF000`. The stricter of PDE and PTE wins.
- **Turn on:** CR3 = physical address of the directory; set CR0.PG (bit 31) and CR0.WP (bit 16,
  486+: read-only applies to ring 0 too).
- **TLB:** after changing an entry, `invlpg [va]` (one page) or reload CR3 (all).
- **Page fault** = vector 14, faulting address in CR2. Error code bit 0 P (1 = protection
  violation, 0 = not present), bit 1 W (write), bit 2 U (ring 3). Example: 5 = present, read,
  user.
- **Frame allocator** (`kernel/pmm.c`): a bitmap over the first 1 GiB (`PMM_LIMIT`), 1 = used;
  frames below 2 MiB (`PMM_FIRST`) are reserved. `pmm_alloc` (first fit, 0 = none),
  `pmm_free` (panics on a double free).
- **Heap** (`lib/heap.c`, `kernel/kheap.c`): implicit free list with boundary tags (header and
  footer), first fit, splitting and coalescing, 8-byte aligned; grows at 0xD0000000 through the
  `kheap_more` callback, which maps frames from `pmm_alloc`.
- `int *volatile p = 0` keeps gcc from turning a visible NULL dereference into `ud2`. Kernels
  that map page 0 build with `-fno-delete-null-pointer-checks`.

## Tasks (Chapter 8)

- `struct task` (`kernel/task.h`): saved `esp`, 8 KiB kernel stack (`TASK_STACK_SIZE`, from
  `kmalloc`), state, `wake_at`, exit status. `tasks[TASK_MAX]`, 16 slots. Task 0 = idle = the
  boot thread; the shell is task 1.
- **States:** READY, RUNNING, SLEEPING (until `wake_at`), BLOCKED (until `task_wake`), ZOMBIE
  (until `task_wait`). A killed task's status is `TASK_KILLED` (−1).
- **`switch_to(&prev->esp, next->esp)`** (`kernel/switch.asm`, 13 instructions): load both
  arguments, push ebp, ebx, esi, edi; save esp; load esp; pop edi, esi, ebx, ebp; `ret`.
- **i386 System V ABI:** EAX, ECX, EDX caller-saved; EBX, ESI, EDI, EBP (and ESP) callee-saved.
  Arguments on the stack, result in EAX.
- **New task:** a fake frame of four zero registers and the return address `task_start`
  (`sti`, then `task_exit(entry())`).
- **Scheduler:** `schedule()` runs with interrupts off; `pick_next()` is round robin with idle as
  the fallback. Preemption: `TASK_SLICE` = 5 ticks (50 ms at 100 Hz) sets `need_resched`;
  `task_preempt()` switches at the end of an IRQ, *after* the EOI.
- **API:** `task_spawn`, `task_yield`, `task_sleep(ticks)`, `task_block`, `task_wake`,
  `task_exit`, `task_wait`, `task_kill`, `task_tick`, `task_preempt`.
- **Critical section on one CPU:** `uint32_t f = irq_save(); ... irq_restore(f);` (`pushf; pop;
  cli`, and `sti` only if IF, bit 9 = 0x200, was set). Nests, unlike bare `cli`/`sti`.
- QEMU (TCG) takes interrupts only between translation blocks, so a race inside one basic block
  can hide in QEMU and still be a bug on hardware.

## User mode (Chapter 9)

- **CPL** = the low two bits of CS (`FROM_USER(r)` = `(r->cs & 3) == 3`). Ring 3 can't run
  privileged instructions (`cli`, `hlt`, `lgdt`, moves to CRn), use ports (IOPL 0 and no I/O
  bitmap), touch pages without U/S, or `int n` on a gate with DPL < 3.
- **TSS:** only ss0 = 0x10 and esp0 matter; `iomap_base = sizeof tss` (no I/O bitmap);
  `ltr 0x28` in `gdt_init`. `schedule()` calls `tss_set_kernel_stack(next->kstack_top)` after
  `current = next`, on every switch.
- **Into ring 3** (`enter_user_mode`, `kernel/usermode.asm`): DS, ES, FS, GS = 0x23; push SS 0x23,
  ESP (`USER_TOP`), EFLAGS (IF and bit 1, IOPL 0), CS 0x1B, EIP; clear the general registers;
  `iret`. **Back to ring 0:** the CPU loads ss0:esp0 and pushes the user SS and ESP, then
  EFLAGS, CS, EIP (`struct regs` ends with `user_esp`, `user_ss`).
- **Address spaces:** one page directory per program; the kernel's entries are shared
  (preallocated tables), 0x40000000–0x80000000 is the program's. ELF files are loaded by
  `elf_load`, which trusts nothing in them.
- **User pointers:** `user_access_ok(p, n, write)` (`kernel/user.c`): inside
  0x40000000–0x80000000, no overflow, every page present and user (and writable for writes);
  otherwise the call returns −EFAULT.
- **Faults:** in ring 3 they kill the task (`task N (name) killed at eip ...`); in ring 0 they
  panic.

**System calls** (`int 0x80`, Linux i386 numbers and registers: number in EAX, arguments in EBX,
ECX, EDX, result in EAX, −errno on error):

| EAX | call | sprout |
|---|---|---|
| 1 | `exit(status)` | ends the task |
| 4 | `write(fd, buf, count)` | fd 1 or 2, to the console |
| 20 | `getpid()` | the task id |
| 158 | `sched_yield()` | `task_yield` |
| 162 | `nanosleep(&ts, rem)` | sleeps; `rem` ignored |
| 3 | `read(fd, buf, count)` | Chapter 9 exercise 1 |
| 45 | `brk(end)` | Chapter 9 exercise 3 |

## Debugging (Chapter 10)

- **Frame chain:** `[ebp]` = the caller's ebp, `[ebp+4]` = the return address. The chain ends
  at ebp = 0 (`entry.asm`, and `task_spawn_in` for new tasks). The walker (`kernel/backtrace.c`)
  checks every frame: non-null, 4-aligned, above the previous one, mapped; at most 32 deep.
  It looks up `ret - 1` and prints `ret`. Through an interrupt it continues at `r->ebp` and
  prints `-- interrupt N --`; it stops at `(user mode)`.
- Inlining and tail calls remove frames: `sys_write` is inlined into `syscall_handler`.
- **Symbols:** `tools/mksyms.sh` (`nm -n --defined-only`, types `t`/`T`, skips `__*`) writes the
  `.ksyms` section. Two links: kernel0.elf with an empty table, then kernel.elf with the real
  one; `mksyms.sh kernel.elf | cmp` checks that no function moved.
- **Address to source:** `addr2line -f -s -e build/kernel.elf ADDR`, add `-i` for inlined
  frames; in gdb `list *cmd_crash+0x77`.

**gdb** (`make debug` in one terminal, `make gdb` in another; `tools/gdbinit` sets
`disassembly-flavor intel`, loads `file build/kernel.elf` and runs `target remote
localhost:1234`):

| command | use |
|---|---|
| `break kmain`, `break *0x7c00` | breakpoint on a function or an address |
| `continue`, `next`, `step`, `stepi`, Ctrl-C | run, step a line, step into, one instruction, stop |
| `bt` | backtrace |
| `info registers`, `info registers eip`, `p $cs & 3` | CPU state; the current ring |
| `x/8i $pc`, `x/4wx ADDR` | disassemble, dump memory |
| `watch tss.esp0` | hardware watchpoint (unlimited under QEMU) |
| `add-symbol-file build/user/count.elf` | a user program's symbols (all are at 0x40000000) |
| `list *func+0xoff` | the source line of an address |
| `dump binary memory build/mem.bin 0x7c00 0x7c20` then `shell ndisasm -b 16 -o 0x7c00 build/mem.bin` | real-mode code (gdb 12.1 shows it as 32-bit) |

Under QEMU, stepping masks interrupts and timers, and gdb can't tell address spaces apart:
check `current->name` and `$cs & 3`.

| symptom | first tool |
|---|---|
| a PANIC with a backtrace | read it bottom-up; `addr2line`, or gdb `list *func+0xoff` |
| silence, and QEMU exits | `QEMUEXTRA="-d int,cpu_reset -D build/qemu.log"`, then the last `v=` lines |
| silence, and QEMU keeps running | `make debug` + `make gdb`, Ctrl-C, `bt`, `info registers` |
| a variable gets a wrong value | gdb `watch` |
| the boot sector or stage 2 | gdb `break *0x7c00` and registers; `ndisasm` for the code |
| wrong only sometimes | interrupts and races (Chapter 8); make it deterministic first |

## Errors you will meet

| message | cause | chapter |
|---|---|---|
| `boot/boot.asm:148: error: TIMES value -36 is negative` | the boot sector is over 510 bytes | 1 |
| `Boot failed: not a bootable disk` | no 0x55 0xAA at offset 510 (missing `times` line or `dw 0xAA55`) | 1 |
| `boot/stage2.asm:159: error: TIMES value -816 is negative` | stage 2 is over 16 sectors: raise `STAGE2_SECTORS` | 2 |
| ``error: symbol `STAGE2_ADDR' not defined before use`` | `org` before the `%include` that defines its value | 2 |
| ``error: label `PRINT' inconsistently redefined`` | a macro used before it is defined | 2 |
| output stops after "entering protected mode" | the far jump after setting CR0.PE is missing | 3 |
| `v=0d e=0018`, `v=08`, `Triple fault` | a selector past the GDT limit, no IDT | 3 |
| ``undefined reference to `__udivdi3'`` | 64-bit division in 32-bit code, no libgcc | 4 |
| ``undefined reference to `__stack_chk_fail_local'`` | built without `-ffreestanding -fno-stack-protector` | 4 |
| ``undefined reference to `__muldf3'`` | floating point with `-mgeneral-regs-only` | 4 |
| ``undefined reference to `memcpy'`` | gcc turned a large struct copy into `memcpy`: define it | 4 |
| no output at all, CPU halted inside `kmain` | linker script address ≠ load address | 4 |
| `unknown conversion type character ‘b’ in format` | a `kprintf` extension checked against printf's format attribute | 4 |
| `exception 13 (general protection), error code 402` | `int 0x80` with no gate (Ch. 5) or from ring 3 through a DPL 0 gate (Ch. 9) | 5, 9 |
| one timer tick, then silence | the IRQ handler forgot the EOI | 5 |
| the timer arrives as "double fault" (vector 8) | the PIC was not remapped | 5 |
| the shell never answers on COM1 | CR not translated to `'\n'` | 6 |
| `page fault: ... at 00000000` | a NULL dereference, page 0 unmapped | 7 |
| a stale value after remapping | missing `invlpg` | 7 |
| crashes far away after a task switch | `switch_to` didn't save a callee-saved register | 8 |
| a user task's interrupt lands on another task's kernel stack | esp0 not updated in `schedule()` | 9 |
| a backtrace of one line | built without `-fno-omit-frame-pointer` | 10 |
| `kernel.elf: functions moved in the second link` | the symbol table changed code addresses | 10 |

## Growth of sprout

`kernel.bin` size, lines in `tests/boot.expected`, host unit tests, from `make test` in each
snapshot:

| chapter | version | kernel.bin | sectors | boot.expected | unit tests |
|---|---|---|---|---|---|
| 1 | 0.1 | – | – | 3 | – |
| 2 | 0.2 | – | – | 13 | – |
| 3 | 0.3 | – | – | 18 | – |
| 4 | 0.4 | 4529 | 9 | 19 | 9 |
| 5 | 0.5 | 7393 | 15 | 27 | 9 |
| 6 | 0.6 | 10185 | 20 | 53 | 16 |
| 7 | 0.7 | 16393 | 33 | 65 | 22 |
| 8 | 0.8 | 20489 | 41 | 76 | 22 |
| 9 | 0.9 | 32777 | 65 | 90 | 22 |
| 10 | 0.10 | 36627 | 72 | 91 | 22 |

## Concept to code

| concept | chapter | where |
|---|---|---|
| BIOS boot, 0x7C00, the 0xAA55 signature | 1 | `boot/boot.asm` |
| COM1 output from real mode | 1–2 | `boot/boot.asm`, `boot/print16.inc` |
| Loading sectors with int 0x13 AH=0x42 | 2 | `boot/boot.asm` (DAP), `boot/layout.inc` |
| E820 memory map | 2 | `boot/stage2.asm`; read by `kernel/memmap.c` (Ch. 7) |
| A20 line | 3 | `boot/a20.inc` |
| GDT and the switch to protected mode | 3 | `boot/gdt.inc`, `boot/stage2.asm` |
| VGA text output | 3–4 | `boot/print32.inc`, `kernel/vga.c` |
| Loading the kernel and copying it to 1 MiB | 4 | `boot/stage2.asm` |
| Freestanding C entry, linker script | 4 | `kernel/entry.asm`, `kernel/linker.ld`, `kernel/kmain.c` |
| Port I/O | 4 | `kernel/io.h` |
| `kprintf` without libc or libgcc | 4 | `lib/fmt.c` (`fmt_divmod64`), `kernel/kprintf.c` |
| Host-tested library code | 4+ | `lib/`, `tests/unit/` |
| IDT, ISR stubs, the interrupt frame | 5 | `kernel/idt.c`, `kernel/isr.asm`, `kernel/interrupts.[ch]` |
| PIC remap and EOI | 5 | `kernel/pic.c` |
| PIT timer, ticks, sleep | 5 | `kernel/timer.c` |
| Keyboard scancodes | 6 | `kernel/keyboard.c` |
| Ring buffer, line editing, arguments | 6 | `lib/ringbuf.c`, `lib/line.c`, `lib/args.c`, `kernel/input.c` |
| COM1 input | 6 | `kernel/serial.c` (`serial_irq`) |
| Shell and command table | 6+ | `kernel/shell.c` |
| Reboot and poweroff | 6 | `kernel/power.c` |
| Physical frame allocator (bitmap) | 7 | `lib/bitmap.c`, `kernel/pmm.c` |
| Paging, page faults | 7 | `kernel/paging.c` |
| Kernel heap (`kmalloc`/`kfree`) | 7 | `lib/heap.c`, `kernel/kheap.c` |
| Tasks, context switch, scheduler | 8 | `kernel/task.c`, `kernel/switch.asm` |
| Zombies, wait, kill | 8 | `kernel/task.c` |
| User and kernel GDT entries, TSS, esp0 | 9 | `kernel/gdt.c` |
| Entering ring 3 | 9 | `kernel/usermode.asm`, `kernel/user.c` |
| Per-task address spaces | 9 | `kernel/paging.c`, `kernel/user.c` |
| System calls (`int 0x80`) | 9 | `kernel/syscall.c`, `user/lib/` |
| ELF loading of user programs | 9 | `kernel/elf.c`, `kernel/userbins.asm`, `kernel/programs.c` |
| Panic and backtrace (frame pointers) | 10 | `kernel/backtrace.c` |
| Kernel symbol table (two links) | 10 | `tools/mksyms.sh`, `kernel/ksyms.c` |
| gdb setup | 10 | `tools/gdbinit` |
| Headless test harness | 1+ | `tools/qemu-test.sh`, `tests/boot.in`, `tests/boot.expected` |
