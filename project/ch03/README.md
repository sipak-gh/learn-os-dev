# sprout — snapshot after Chapter 3

Stage 2 now switches the CPU to 32-bit protected mode: it checks (and if
needed enables) the A20 line, loads a GDT, sets CR0.PE and far-jumps into 32-bit
code, which prints with its own VGA and COM1 routines and tests memory above
1 MiB.

## Build and run

Requirements: NASM 3.02 (`export PATH=$HOME/opt/nasm/bin:$PATH` if that is
where yours lives), GNU make, and QEMU (`qemu-system-i386`).

- `make` builds `build/sprout.img`, a 1 MiB raw disk image.
- `make run` boots it in a QEMU window, with COM1 on your terminal.
- `make run-nox` shows the VGA screen in the terminal instead (curses; quit
  with Esc, 2, then `q` in the monitor).
- `make test` boots headless and compares COM1 with `tests/boot.expected`.
- `make demo` boots headless and prints the COM1 transcript.
- `make disasm` disassembles the loader.
- `make clean` removes `build/`.

```console
$ make test
sh tools/qemu-test.sh "qemu-system-i386 -m 128M" build/sprout.img tests/boot.expected
PASS: build/sprout.img (18 lines of serial output match tests/boot.expected)
$ make demo
...
sprout: usable memory: 130559 KiB
sprout: A20 line already enabled
sprout: entering protected mode
sprout: hello from 32-bit protected mode
sprout: CR0 = 0x00000011, CS = 0x00000008, DS = 0x00000010
sprout: wrote and read back 0x100000: 0x12345678
sprout: stage 2 done, halting
```

## What is here

| Path | What |
|---|---|
| `boot/a20.inc` | test the A20 line; enable it (BIOS, then the fast A20 port) |
| `boot/gdt.inc` | a flat GDT: null, code 0x08, data 0x10 |
| `boot/print32.inc` | protected-mode output: VGA text memory and COM1 |
| `boot/stage2.asm` | E820, A20, GDT, the switch, then 32-bit code |

The other files come from Chapter 2; `diff -ru ../ch02 .` shows every change.

## What changed in this chapter

- `boot/a20.inc`, `boot/gdt.inc` and `boot/print32.inc` are new.
- `boot/stage2.asm` gains the switch to protected mode and the 32-bit part.
