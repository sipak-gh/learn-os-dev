# gdbuser.gdb — debug a user program (make gdbuser).
# The Makefile types "run count", then "spawn spin", into COM1.

add-symbol-file build/user/count.elf
break main
continue
print $cs & 3
printf "%s\n", current->name
print &n
next
next
next
next
next
print n
print pid

# count is done; spin starts. Its main is at the same address as
# count's, in another address space, and gdb can't tell them apart.
continue
info registers eip cs
x/2i $pc
printf "%s\n", current->name
kill
