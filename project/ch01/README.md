# sprout — snapshot after Chapter 1

sprout is the course's kernel. Here it is still a seed: a 512-byte boot
sector that the BIOS loads at 0x7C00. It prints a greeting and the BIOS boot
drive on the screen and on COM1 (the serial port), then halts.

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
$ make
$ make test
sh tools/qemu-test.sh "qemu-system-i386" build/sprout.img tests/boot.expected
PASS: build/sprout.img (3 lines of serial output match tests/boot.expected)
$ make demo
sprout: hello from the boot sector
sprout: BIOS boot drive 0x80
sprout: halting
```

## What is here

| Path | What |
|---|---|
| `Makefile` | assemble the boot sector, pad it to a 1 MiB disk image; run, test, demo |
| `boot/boot.asm` | the boot sector: segments, stack, print via BIOS and COM1, halt, `55 AA` |
| `tests/boot.expected` | what COM1 must show |
| `tools/qemu-test.sh` | boot headless, capture COM1, diff with the expected output |

## What changed in this chapter

Everything is new.
