# Makefile — Chapter 4 examples: what goes wrong when C meets bare metal.
#
#   make divide      64-bit division: ld can't find __udivdi3
#   make canary      a hosted build: ld can't find __stack_chk_fail_local
#   make pie         the same function compiled with and without -fno-pie
#   make wronglink   the Chapter 4 kernel linked for 2 MiB but run at 1 MiB
#   make clean

QEMU ?= qemu-system-i386

# The kernel's flags (see project/ch04/Makefile).
KCFLAGS := -m32 -ffreestanding -fno-pie -fno-stack-protector \
           -fcf-protection=none -fno-asynchronous-unwind-tables \
           -mgeneral-regs-only -std=c11 -O2 -Wall -Wextra
LDFLAGS := -m elf_i386 -nostdlib

divide:
	gcc $(KCFLAGS) -c divide.c -o divide.o
	ld $(LDFLAGS) -e average -o divide.elf divide.o

# No -ffreestanding and no -fno-stack-protector: what you get when you
# compile kernel code like an ordinary program (on Ubuntu, Debian, ...).
canary:
	gcc -m32 -O2 -c canary.c -o canary.o
	objdump -dr -M intel --no-show-raw-insn canary.o | grep -E 'gs:|stack_chk'
	ld $(LDFLAGS) -e name_length -o canary.elf canary.o

pie:
	gcc $(filter-out -fno-pie,$(KCFLAGS)) -c counter.c -o counter-pie.o
	gcc $(KCFLAGS) -c counter.c -o counter.o
	@echo "--- with PIE (gcc's default):"
	@objdump -dr -M intel --no-show-raw-insn counter-pie.o | sed -n '/<tick>:/,$$p'
	@echo "--- with -fno-pie:"
	@objdump -dr -M intel --no-show-raw-insn counter.o | sed -n '/<tick>:/,$$p'

# Build a copy of the Chapter 4 project with one change in linker.ld,
# boot it for 3 seconds, then ask the QEMU monitor where the CPU is.
wronglink:
	rm -rf build/wronglink && mkdir -p build
	cp -r ../../project/ch04 build/wronglink
	$(MAKE) -C build/wronglink clean
	sed -i 's/\. = 0x100000;/. = 0x200000;/' build/wronglink/kernel/linker.ld
	grep -n '0x200000' build/wronglink/kernel/linker.ld
	$(MAKE) -s -C build/wronglink
	(sleep 3; printf 'info registers\nquit\n') | timeout 6 $(QEMU) -m 128M \
	    -drive format=raw,file=build/wronglink/build/sprout.img -no-reboot \
	    -display none -serial file:build/serial.log -monitor stdio \
	    | grep -E '^E(AX|SI|IP)'
	@echo "--- COM1:"; tr -d '\r' < build/serial.log
	@echo "--- symbols near EIP (link address = EIP + 0x100000):"
	nm -n build/wronglink/build/kernel.elf | grep -E ' (kmain|_start)$$'

clean:
	rm -rf build *.o *.elf

.PHONY: divide canary pie wronglink clean
