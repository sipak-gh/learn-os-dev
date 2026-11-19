# sprout — snapshot after Chapter 8

sprout runs several tasks: each has its own kernel stack, `switch_to`
(assembly) swaps them, and the timer preempts them round robin every 50 ms. The
shell is task 1 and can start, list, kill and wait for tasks (`run`, `spawn`,
`ps`, `kill`, `wait`).

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
PASS: build/sprout.img (76 lines of serial output match tests/boot.expected)
$ make demo
...
tasks: round robin, 50 ms time slice; the shell is task 1
type help for a list of commands
sprout> run ping pong
ping 1
pong 1
...
task 2 (ping) exited with status 0
task 3 (pong) exited with status 0
...
```

## What is here

| Path | What |
|---|---|
| `kernel/task.[ch]` | tasks, the scheduler, sleeping, exit, kill and wait |
| `kernel/switch.asm` | `switch_to`: save one task's registers, load another's |
| `kernel/programs.[ch]` | the kernel programs the shell can run (ping, pong, tick, tock, hog) |

The other files come from Chapter 7; `diff -ru ../ch07 .` shows every change.

## What changed in this chapter

- New: `kernel/{task,programs}.[ch]` and `kernel/switch.asm`.
- The timer drives the scheduler; a task that sleeps or waits for input is blocked
  instead of spinning.
- The shell is `shell_main`, task 1, with `run`, `spawn`, `ps`, `kill` and `wait`.
  The version is 0.8.
