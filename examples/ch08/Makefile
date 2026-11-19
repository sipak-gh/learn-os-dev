# Makefile — Chapter 8 examples: what the scheduler needs, by taking it away.
#
# Each target copies the Chapter 8 snapshot to build/NAME, changes one
# thing (the diff is printed), builds it and boots it headless.
#
#   make nopreempt  no task switch from interrupt handlers: cooperative only
#   make race       two tasks add to one counter without disabling interrupts
#   make racehidden the same race inside one basic block: QEMU never shows it
#   make nosave     switch_to forgets to save ebx, esi and edi
#   make clean

QEMU ?= qemu-system-i386
PROJECT := ../../project/ch08
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

nopreempt:
	$(call copy,$@)
	sed -i '/^    task_preempt();$$/d' build/$@/kernel/interrupts.c
	$(call build,$@)
	(sleep 2; printf 'run ping pong\rrun tick tock\rspawn hog\r'; sleep 1; \
	 printf 'echo hello?\r'; sleep 1; printf 'ps\r'; sleep 1) \
	    | timeout 6 $(QEMU) -m 128M $(IMG) -display none -no-reboot \
	    -serial stdio -monitor none 2>/dev/null | tr -d '\r' | sed -n '/^sprout> /,$$p'
	@echo
	@echo "--- (no answer: the shell is ready, but hog never gives up the CPU)"

race:
	$(call copy,$@)
	sed -i 's|^static const struct program programs\[\] = {|static int count(void);\n\n&\n    { "count", count },|' \
	    build/$@/kernel/programs.c
	cat count.c >>build/$@/kernel/programs.c
	$(call build,$@)
	$(call type,run count count\r,3)

racehidden:
	$(call copy,$@)
	sed -i 's|^static const struct program programs\[\] = {|static int count(void);\n\n&\n    { "count", count },|' \
	    build/$@/kernel/programs.c
	sed -e '/^__attribute__((noinline))/,/^}/d' \
	    -e 's|counter = plus_one(counter);    /\* load, call, store \*/|counter = counter + 1;              /* load, add, store */|' \
	    count.c >>build/$@/kernel/programs.c
	$(call build,$@)
	objdump -d --no-show-raw-insn build/$@/build/kernel.elf | sed -n '/<count>:/,/ret/p'
	$(call type,run count count\r,3)

nosave:
	$(call copy,$@)
	sed -i -e '/^    push ebx$$/,/^    push edi$$/c\    sub esp, 12                 ; room for them, but nothing saved' \
	    -e '/^    pop edi$$/,/^    pop ebx$$/c\    add esp, 12' build/$@/kernel/switch.asm
	$(call build,$@)
	$(call type,run ping pong\rrun tick tock\rps\r,3)

clean:
	rm -rf build

.PHONY: nopreempt race racehidden nosave clean
