# sprout — snapshot after Chapter 9

sprout runs programs in ring 3. Each user program is a separate ELF file,
loaded into its own address space at 0x40000000, and can reach the kernel only
through system calls (`int 0x80`, with Linux's i386 numbers). A program that
faults is killed; the kernel carries on.

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
unit: 22 passed, 0 failed
PASS: build/sprout.img (90 lines of serial output match tests/boot.expected)
$ make demo
...
user mode: programs run in ring 3 at 0x40000000, system calls via int 0x80
type help for a list of commands
sprout> run hello
hello from ring 3
task 2 (hello) exited with status 0
...
sprout> run priv
priv: trying cli
exception 13 (general protection), error code 0
task 6 (priv) killed at eip 40000096
task 6 (priv) was killed
...
```

## What is here

| Path | What |
|---|---|
| `kernel/gdt.[ch]` | the kernel's own GDT with ring 3 segments, and the TSS |
| `kernel/usermode.asm` | `enter_user_mode`: `iret` down to ring 3 |
| `kernel/elf.[ch]`, `kernel/user.[ch]` | load an ELF program into a new address space and start it |
| `kernel/syscall.[ch]` | `int 0x80`: exit, write, getpid, sched_yield, nanosleep |
| `kernel/userbins.asm` | the user programs, embedded in the kernel image |
| `user/` | the programs (`hello`, `snoop`, `priv` in NASM; `count`, `spin` in C) and their tiny C library |

The other files come from Chapter 8; `diff -ru ../ch08 .` shows every change.

## What changed in this chapter

- New: `kernel/{gdt,elf,user,syscall}.[ch]`, `kernel/{usermode,userbins}.asm` and `user/`.
- `paging.c` creates one address space per program and checks user pointers;
  `task.c` sets the TSS's kernel stack on every switch.
- The Makefile builds the user programs first. The version is 0.9.
