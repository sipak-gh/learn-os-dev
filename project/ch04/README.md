# sprout — snapshot after Chapter 4

sprout gets a kernel in C. Stage 2 loads it from the disk to 0x10000, switches
to protected mode, copies it to 1 MiB and jumps to it with a magic number in EAX
and the boot information in EBX. The kernel prints on VGA and COM1 with its
own `kprintf`. Code without hardware access lives in `lib/` and is unit-tested
on the host.

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
$ make
...
kernel.bin: 4529 bytes, 9 sectors
$ make test
...
unit: 9 passed, 0 failed
sh tools/qemu-test.sh "qemu-system-i386 -m 128M" build/sprout.img tests/boot.expected
PASS: build/sprout.img (19 lines of serial output match tests/boot.expected)
$ make demo
...
sprout: protected mode; copying kernel to 0x100000
sprout 0.4
kernel: running at 0x00100000, booted from drive 80
memory map (6 entries from E820):
...
```

## What is here

| Path | What |
|---|---|
| `boot/` | the loader; stage 2 now loads, copies and starts the kernel |
| `kernel/entry.asm` | `_start`: stack, zero `.bss`, call `kmain` |
| `kernel/linker.ld` | link the kernel at 0x100000 |
| `kernel/kmain.c` | the kernel's `main` |
| `kernel/{serial,vga,kprintf}.[ch]` | COM1, VGA text mode, formatted output to both |
| `kernel/io.h`, `kernel/boot_info.h` | `inb`/`outb`; what stage 2 hands over |
| `lib/{string,fmt}.[ch]` | freestanding `mem*`/`str*` and number formatting |
| `tests/unit/` | unit tests for `lib/`, run on the host |

The other files come from Chapter 3; `diff -ru ../ch03 .` shows every change.

## What changed in this chapter

- `kernel/`, `lib/` and `tests/unit/` are new.
- `boot/stage2.asm` prints the memory map as one summary line, loads the kernel
  and jumps to it; `boot/layout.inc` gains the kernel's addresses and `BOOT_MAGIC`.
- The Makefile builds the kernel with `gcc -m32 -ffreestanding` and `ld -m elf_i386`,
  and the unit tests with the host compiler.
