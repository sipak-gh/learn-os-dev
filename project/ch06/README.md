# sprout — snapshot after Chapter 6

sprout gets a shell. Keyboard (IRQ 1) and COM1 (IRQ 4) input go through one
ring buffer to a line editor, and the shell runs commands such as `help`,
`echo`, `mem`, `uptime`, `sleep`, `clear`, `reboot` and `poweroff`.

## Build and run

Requirements: NASM 3.02 (`export PATH=$HOME/opt/nasm/bin:$PATH` if that is
where yours lives), gcc with `-m32` support (no multilib or 32-bit libgcc
needed: the kernel never links libgcc), GNU ld and make, and QEMU
(`qemu-system-i386`).

- `make` builds `build/sprout.img`, a 1 MiB raw disk image.
- `make run` boots it in a QEMU window, with COM1 on your terminal.
- `make run-nox` shows the VGA screen in the terminal instead (curses; quit
  with Esc, 2, then `q` in the monitor).
  Type commands in the QEMU window or on the terminal (COM1): both work.
- `make test` runs the unit tests of `lib/` on the host, then boots headless and
  compares COM1 with `tests/boot.expected`, typing `tests/boot.in` into
  COM1.
- `make demo` boots headless and prints the COM1 transcript.
- `make disasm` disassembles the loader and the kernel.
- `make clean` removes `build/`.

```console
$ make test
...
unit: 16 passed, 0 failed
PASS: build/sprout.img (53 lines of serial output match tests/boot.expected)
$ make demo
...
interrupts on: timer 100 Hz, keyboard (IRQ 1), COM1 input (IRQ 4)
type help for a list of commands
sprout> echo hello,   world
hello, world
sprout> version
sprout 0.6
...
```

## What is here

| Path | What |
|---|---|
| `kernel/keyboard.[ch]` | the PS/2 keyboard: scancode set 1 to characters |
| `kernel/input.[ch]` | one input queue for keyboard and COM1 |
| `kernel/shell.[ch]` | the command table and the read–run loop |
| `kernel/memmap.[ch]`, `kernel/power.[ch]` | the E820 map; reboot and poweroff |
| `lib/{ringbuf,line,args}.[ch]` | ring buffer, line editing, splitting words (unit-tested) |
| `tests/boot.in` | what `make test` and `make demo` type into COM1 |

The other files come from Chapter 5; `diff -ru ../ch05 .` shows every change.

## What changed in this chapter

- New: `kernel/{keyboard,input,shell,memmap,power}.[ch]`, `lib/{ringbuf,line,args}.[ch]`,
  `tests/unit/test_input.c`, `tests/boot.in`.
- `kernel/serial.c` enables the receive interrupt; `tools/qemu-test.sh` types
  `tests/boot.in`, one line per prompt. The version is 0.6.
