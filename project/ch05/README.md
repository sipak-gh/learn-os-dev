# sprout — snapshot after Chapter 5

sprout handles interrupts: an IDT with all 32 CPU exceptions and 16 hardware
IRQs, the 8259 PIC remapped to vectors 32–47, and the PIT timer at 100 Hz. An
unexpected exception prints a register dump and panics instead of rebooting.

## Build and run

Requirements: NASM 3.02 (`export PATH=$HOME/opt/nasm/bin:$PATH` if that is
where yours lives), gcc with `-m32` support (no multilib or 32-bit libgcc
needed: the kernel never links libgcc), GNU ld and make, and QEMU
(`qemu-system-i386`).

- `make` builds `build/sprout.img`, a 1 MiB raw disk image.
- `make run` boots it in a QEMU window, with COM1 on your terminal.
- `make run-nox` shows the VGA screen in the terminal instead (curses; quit
  with Esc, 2, then `q` in the monitor).
- `make test` runs the unit tests of `lib/` on the host, then boots headless and
  compares COM1 with `tests/boot.expected`.
- `make demo` boots headless and prints the COM1 transcript.
- `make disasm` disassembles the loader and the kernel.
- `make clean` removes `build/`.

```console
$ make test
...
unit: 9 passed, 0 failed
PASS: build/sprout.img (27 lines of serial output match tests/boot.expected)
$ make demo
...
idt: vectors 0-31 (exceptions) and 32-47 (IRQs) installed
pic: IRQ 0-15 remapped to vectors 32-47
breakpoint: vector 3 from cs=0008, byte before eip: cc
back in kmain after int3
timer: PIT channel 0 at 100 Hz, interrupts enabled
  0.5 s
  1.0 s
  1.5 s
idle: halting until the next interrupt, forever
```

## What is here

| Path | What |
|---|---|
| `kernel/isr.asm` | 48 interrupt entry stubs and `isr_common` |
| `kernel/idt.c` | the IDT |
| `kernel/interrupts.[ch]` | `struct regs`, dispatch, handlers, the panic dump |
| `kernel/pic.[ch]` | the 8259 PIC: remap, mask, end of interrupt |
| `kernel/timer.[ch]` | the PIT at 100 Hz, ticks and `sleep` |

The other files come from Chapter 4; `diff -ru ../ch04 .` shows every change.

## What changed in this chapter

- `kernel/isr.asm`, `idt.c`, `interrupts.[ch]`, `pic.[ch]` and `timer.[ch]` are new.
- `kmain` sets up interrupts, tests `int3` and the timer, then idles with `hlt`.
- The Makefile assembles every `kernel/*.asm`. The version is 0.5.
