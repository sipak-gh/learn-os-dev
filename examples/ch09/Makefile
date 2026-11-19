# Makefile — Chapter 9 examples: what keeps ring 3 in its place, by
# taking it away.
#
# Each target copies the Chapter 9 snapshot to build/NAME, changes one
# thing (the diff is printed), builds it and boots it headless.
#
#   make dpl0       the int 0x80 gate has DPL 0: ring 3 may not use it
#   make nouser     user pages are mapped without the U/S bit
#   make esp0once   the TSS's esp0 is set when a program starts, not on
#                   every task switch
#   make nocheck    write() trusts the pointer it is given
#   make clean

QEMU ?= qemu-system-i386
PROJECT := ../../project/ch09
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
# up, wait SECONDS, and show COM1 from the first prompt on.
define type
	(sleep 2; printf '$(1)'; sleep $(2)) | timeout $$((3 + $(2))) $(QEMU) -m 128M $(IMG) \
	    -display none -no-reboot -serial stdio -monitor none 2>/dev/null \
	    | tr -d '\r' | sed -n '/^sprout> /,$$p'
endef

dpl0:
	$(call copy,$@)
	sed -i 's/^#define GATE_TRAP32_USER 0xEF/#define GATE_TRAP32_USER 0x8F/' \
	    build/$@/kernel/idt.c
	$(call build,$@)
	$(call type,run hello\r,1)

nouser:
	$(call copy,$@)
	sed -i 's/table\[TABLE_INDEX(virt)\] = phys | flags | PAGE_USER | PAGE_PRESENT;/table[TABLE_INDEX(virt)] = phys | flags | PAGE_PRESENT;/' \
	    build/$@/kernel/paging.c
	$(call build,$@)
	$(call type,run hello\r,1)

esp0once:
	$(call copy,$@)
	sed -i '/^    if (next->stack)$$/,/^        tss_set_kernel_stack/d' build/$@/kernel/task.c
	sed -i -e 's/^#include "elf.h"$$/&\n#include "gdt.h"/' \
	    -e 's/^    enter_user_mode(task_current()->arg, USER_TOP);/    struct task *me = task_current();\n    tss_set_kernel_stack((uint32_t)me->stack + TASK_STACK_SIZE);\n    enter_user_mode(me->arg, USER_TOP);/' \
	    build/$@/kernel/user.c
	$(call build,$@)
	$(call type,run hello\rrun count count\r,3)
	@echo
	@echo "--- (esp0 is count 4's stack for both programs: count 3's interrupts and"
	@echo "---  system calls land on top of what count 4 left there while it sleeps)"

nocheck:
	$(call copy,$@)
	sed -i '/^    if (!user_access_ok((const void \*)buf, count, false))$$/,/^        return -EFAULT;$$/d' \
	    build/$@/kernel/syscall.c
	$(call build,$@)
	$(call type,run snoop\r,1)

clean:
	rm -rf build

.PHONY: dpl0 nouser esp0once nocheck clean
