# sprout — snapshot after Chapter 7

sprout manages memory: a bitmap allocator for 4 KiB physical frames, paging
(RAM identity-mapped, page 0 unmapped so NULL faults, kernel code read-only)
and a kernel heap (`kmalloc`/`kfree`) at 0xd0000000. Page faults are explained
in words. New shell commands: `vm`, `heap`, `alloc` and `free`; `crash` can
now also follow a NULL pointer or write to code.

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
PASS: build/sprout.img (65 lines of serial output match tests/boot.expected)
$ make demo
...
frames: 32192 of 32224 free (128768 KiB), 4 KiB each, 00200000-07fe0000
paging: on; RAM identity-mapped up to 07fe0000 except page 0, kernel code read-only
heap: kmalloc from 0xd0000000
...
sprout> alloc 100
kmalloc(100) = 0xd0000010
...
```

## What is here

| Path | What |
|---|---|
| `kernel/pmm.[ch]` | the physical frame allocator |
| `kernel/paging.[ch]` | page directory and tables, the page fault handler |
| `kernel/kheap.[ch]` | `kmalloc`/`kfree` on pages mapped at 0xd0000000 |
| `lib/{bitmap,heap}.[ch]` | the bitmap and the heap's free list (unit-tested) |

The other files come from Chapter 6; `diff -ru ../ch06 .` shows every change.

## What changed in this chapter

- New: `kernel/{pmm,paging,kheap}.[ch]`, `lib/{bitmap,heap}.[ch]`, `tests/unit/test_memory.c`.
- `kernel/memmap.c` gives usable frames to the allocator; `linker.ld` puts
  writable data on its own pages, so code and constants can be mapped read-only.
- The shell gains `vm`, `heap`, `alloc` and `free`, and `crash [div|null|code]`.
  The version is 0.7.
