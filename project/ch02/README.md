# sprout — snapshot after Chapter 2

The boot sector is now a loader: it reads an 8 KiB stage 2 from the disk
with the BIOS (INT 13h extensions) and jumps to it. Stage 2 asks the BIOS for
the memory map (INT 15h, E820) and prints it.

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
PASS: build/sprout.img (13 lines of serial output match tests/boot.expected)
$ make demo
sprout: boot sector, loading stage 2
sprout: stage 2 loaded, jumping to it
sprout: stage 2 running at 0x8000
sprout: E820 memory map (6 entries)
  base                length              type
  0x0000000000000000  0x000000000009fc00  1 usable
  0x000000000009fc00  0x0000000000000400  2 reserved
  0x00000000000f0000  0x0000000000010000  2 reserved
  0x0000000000100000  0x0000000007ee0000  1 usable
  0x0000000007fe0000  0x0000000000020000  2 reserved
  0x00000000fffc0000  0x0000000000040000  2 reserved
sprout: usable memory: 130559 KiB
sprout: stage 2 done, halting
```

## What is here

| Path | What |
|---|---|
| `boot/boot.asm` | the boot sector: load stage 2 (LBA 1–16) to 0x8000 and jump to it |
| `boot/stage2.asm` | stage 2: the E820 memory map |
| `boot/layout.inc` | every shared number: LBAs, addresses, the `BOOT_INFO` structure |
| `boot/print16.inc` | real-mode output to the screen and COM1: strings, hex, decimal |

The other files come from Chapter 1; `diff -ru ../ch01 .` shows every change.

## What changed in this chapter

- `boot/boot.asm` loads stage 2 instead of printing and halting.
- `boot/stage2.asm`, `boot/layout.inc` and `boot/print16.inc` are new.
- The Makefile builds both stages and concatenates them into the image.
- QEMU runs with `-m 128M` (the memory map in `tests/boot.expected` depends on it).
