# Makefile — Chapter 10 examples: finding bugs in sprout.
#
# Each target copies the Chapter 10 snapshot to build/NAME, changes one
# thing or nothing (the diff is printed), builds it and boots it headless.
#
#   make crash      a kernel bug's panic: backtrace, then addr2line and gdb
#   make nofp       the same panic built without frame pointers
#   make sysbug     a panic inside a system call made by a user program
#   make triple     the classic unpacked struct idt_ptr: a triple fault,
#                   hunted with -d int,cpu_reset
#   make gdbboot    gdb: from the reset vector to kmain
#   make gdbkernel  gdb: a breakpoint on the page fault handler
#   make gdbuser    gdb: a breakpoint in a user program
#   make clean
#
# The gdb examples run QEMU with -s -S, like make debug, so they need
# localhost:1234 to be free.

QEMU ?= qemu-system-i386
GDB ?= gdb
PROJECT := ../../project/ch10
IMG = -drive format=raw,file=build/$@/build/sprout.img

# $(call copy,NAME): a fresh copy of the snapshot in build/NAME.
define copy
	rm -rf build/$(1) && mkdir -p build
	cp -r $(PROJECT) build/$(1)
	rm -rf build/$(1)/build
endef

# $(call build,NAME): show what changed, then build quietly.
define build
	diff -ru -x build $(PROJECT) build/$(1) || true
	$(MAKE) -s -C build/$(1)
endef

# $(call type,TEXT,SECONDS): boot, type TEXT into COM1 once the shell is
# up, wait SECONDS, and show COM1 from the first prompt on (and keep it
# in build/NAME/serial.log).
define type
	(sleep 2; printf '$(1)'; sleep $(2)) | timeout $$((3 + $(2))) $(QEMU) -m 128M $(IMG) \
	    -display none -no-reboot -serial stdio -monitor none 2>/dev/null \
	    | tr -d '\r' | sed -n '/^sprout> /,$$p' | tee build/$@/serial.log
endef

# $(call gdb,TEXT): boot stopped (-s -S), type TEXT into COM1 3 seconds
# after gdb lets the machine go, and run gdb with tools/gdbinit and
# NAME.gdb. Then show COM1 (from the first prompt on, if there is one).
define gdb
	cd build/$@ && { (sleep 3; printf '$(1)'; sleep 8) | timeout 30 $(QEMU) -m 128M \
	    -drive format=raw,file=build/sprout.img -display none -no-reboot \
	    -serial stdio -monitor none -s -S >serial.log 2>&1 & \
	    sleep 1; timeout 30 $(GDB) -q -batch -x tools/gdbinit -x ../../$@.gdb; wait; }
	@echo "--- COM1:"
	@tr -d '\r' < build/$@/serial.log | if grep -q '^sprout> ' build/$@/serial.log; \
	    then sed -n '/^sprout> /,$$p'; else cat; fi
endef

crash:
	$(call copy,$@)
	$(call build,$@)
	$(call type,crash null\r,1)
	@echo
	@echo "--- where in cmd_crash? addr2line turns the address into a line:"
	cd build/$@ && addr2line -f -s -e build/kernel.elf \
	    $$(sed -n 's/^  \([0-9a-f]*\)  cmd_crash+.*/\1/p' serial.log)
	cd build/$@ && sed -n "$$(addr2line -e build/kernel.elf \
	    $$(sed -n 's/^  \([0-9a-f]*\)  cmd_crash+.*/\1/p' serial.log) | cut -d: -f2)p" kernel/shell.c
	@echo
	@echo "--- or let gdb read name+offset straight from the backtrace:"
	cd build/$@ && $(GDB) -q -batch build/kernel.elf \
	    -ex "list *$$(sed -n 's/^  [0-9a-f]*  \(cmd_crash+0x[0-9a-f]*\)$$/\1/p' serial.log)"

nofp:
	$(call copy,$@)
	sed -i 's/ -fno-omit-frame-pointer -std=c11/ -std=c11/' build/$@/Makefile
	$(call build,$@)
	$(call type,crash null\r,1)

sysbug:
	$(call copy,$@)
	sed -i '/user_access_ok((const void \*)buf, count, false)/{n;s/$$/\n    kassert(count < 16);/}' \
	    build/$@/kernel/syscall.c
	$(call build,$@)
	$(call type,run hello\r,1)
	@echo
	@echo "--- the user program's side: eip is the instruction after its int 0x80"
	objdump -d -M intel --no-show-raw-insn build/$@/build/user/hello.elf | grep -A1 "int    0x80" | head -2

triple:
	$(call copy,$@)
	sed -i '/^struct idt_ptr {/,/^}/s/^} __attribute__((packed));/};/' build/$@/kernel/idt.c
	$(call build,$@)
	timeout 5 $(QEMU) -m 128M $(IMG) -display none -no-reboot -serial stdio -monitor none \
	    -d int,cpu_reset -D build/$@/qemu.log 2>/dev/null | tr -d '\r' | tail -3
	@echo
	@echo "--- QEMU exited: a triple fault resets the CPU, and -no-reboot turns that into an exit."
	@echo "--- build/$@/qemu.log has $$(wc -l < build/$@/qemu.log) lines; the interrupts and exceptions:"
	grep -E ': v=|check_exception|Triple fault' build/$@/qemu.log
	@echo
	@echo "--- the IDT register at the first interrupt, and where idt really is:"
	sed -n '/ v=20 /,/^IDT=/{/^IDT=/p}' build/$@/qemu.log
	nm build/$@/build/kernel.elf | grep ' idt$$'
	@echo
	@echo "--- the interrupted instruction:"
	addr2line -f -s -e build/$@/build/kernel.elf \
	    $$(sed -n 's/.* v=20 .* pc=\([0-9a-f]*\) .*/\1/p' build/$@/qemu.log)

gdbboot:
	$(call copy,$@)
	$(call build,$@)
	$(call gdb,)

gdbkernel:
	$(call copy,$@)
	$(call build,$@)
	$(call gdb,run snoop\r)

gdbuser:
	$(call copy,$@)
	$(call build,$@)
	$(call gdb,run count\rspawn spin\r)

clean:
	rm -rf build

.PHONY: crash nofp sysbug triple gdbboot gdbkernel gdbuser clean
