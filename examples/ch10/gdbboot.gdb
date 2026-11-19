# gdbboot.gdb — from the reset vector to kmain (make gdbboot).
# tools/gdbinit has already loaded the kernel's symbols and connected.

# QEMU stopped before the first instruction: the reset vector, F000:FFF0.
info registers eip cs

# The BIOS has loaded our boot sector and jumps to it with the boot
# drive in DL.
break *0x7c00
continue
info registers eip cs dl
x/2xb 0x7dfe

# gdb disassembles real-mode code as 32-bit code: wrong.
x/3i $pc
# So let ndisasm do it.
dump binary memory build/mem.bin 0x7c00 0x7c11
shell ndisasm -b 16 -o 0x7c00 build/mem.bin

# Stage 2, then the kernel, in protected mode, with symbols.
break *0x8000
continue
break kmain
continue
info registers eip cs cr0
print/x magic
print *bi
bt
kill
