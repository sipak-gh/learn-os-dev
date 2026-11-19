# sprout — snapshot after Chapter 10

The final sprout. A panic now prints a backtrace with function names, from
the frame-pointer chain and a symbol table the kernel carries inside itself
(the kernel is linked twice to make it). `make debug` and `make gdb` attach gdb
to the running machine.

## Build and run

Requirements: NASM 3.02 (`export PATH=$HOME/opt/nasm/bin:$PATH` if that is
where yours lives), gcc with `-m32` support (no multilib or 32-bit libgcc
needed: the kernel never links libgcc), GNU ld and make, QEMU
(`qemu-system-i386`) and gdb.

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
- `make debug` starts QEMU stopped at the first instruction, with a gdb
  server on localhost:1234; `make gdb`, in a second terminal, connects gdb
  with the kernel's symbols (`tools/gdbinit`). Headless:
  `make debug QEMUEXTRA="-display none"`.
- `QEMUEXTRA` adds QEMU options to `run`, `run-nox` and `debug`, for example
  `make run QEMUEXTRA="-d int,cpu_reset -D build/qemu.log"`.
- `make clean` removes `build/`.

```console
$ make test
...
unit: 22 passed, 0 failed
PASS: build/sprout.img (91 lines of serial output match tests/boot.expected)
$ make demo
...
user mode: programs run in ring 3 at 0x40000000, system calls via int 0x80
panics print a backtrace, with function names
type help for a list of commands
...
$ make run
...
sprout> crash null
...
PANIC at kernel/paging.c:175: unhandled page fault
backtrace:
  0010156d  page_fault+0x6d
  001001b7  isr_common+0x1a
  -- interrupt 14 --
  001023d7  cmd_crash+0x77
  00102a70  shell_main+0xc0
  0010305f  task_start+0xf
system halted
```

## What is here

| Path | What |
|---|---|
| `kernel/backtrace.[ch]` | walk the frame-pointer chain, through interrupt frames |
| `kernel/ksyms.[ch]` | look up an address in the kernel's symbol table |
| `tools/mksyms.sh` | make the symbol table (NASM source) from `nm` |
| `tools/gdbinit` | gdb setup for `make gdb` |

The other files come from Chapter 9; `diff -ru ../ch09 .` shows every change.

## What changed in this chapter

- New: `kernel/{backtrace,ksyms}.[ch]`, `tools/mksyms.sh`, `tools/gdbinit`.
- `panic` prints a backtrace. The kernel is compiled with `-fno-omit-frame-pointer`
  and linked twice (`kernel0.elf`, then `kernel.elf` with the symbol table).
- The Makefile gains `make debug`, `make gdb` and `QEMUEXTRA`. The version is 0.10.
- The addresses in a backtrace depend on your compiler; the names don't.
