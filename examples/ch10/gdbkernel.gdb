# gdbkernel.gdb — stop the kernel on a page fault (make gdbkernel).
# The Makefile types "run snoop" into COM1 a few seconds after boot.

break page_fault
continue

# Where are we, and who called us? gdb stops at isr_common: it can't
# see through the interrupt frame. sprout's backtrace can.
bt
print/x *r
print/x r->cs
print/x r->error_code
print/x $cr2
printf "%s\n", current->name

# The faulting instruction is in the user program, which gdb knows
# nothing about until we load its symbols.
x/i r->eip
add-symbol-file build/user/snoop.elf
x/i r->eip
info line *r->eip
kill
