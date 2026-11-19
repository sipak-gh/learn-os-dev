# Makefile — Chapter 5 examples: interrupts done wrong, on purpose.
#
# Each target copies project/ch05 to build/NAME, changes one thing (the
# diff is printed), builds it and boots it headless for a few seconds.
#
#   make divzero    divide by zero in kmain: the register dump, and addr2line
#   make notpresent int 0x80 with no gate: a general protection fault
#   make noeoi      the IRQ handler never sends EOI: one tick, then silence
#   make noremap    no pic_init: the timer arrives as vector 8, "double fault"
#   make clean

QEMU ?= qemu-system-i386
PROJECT := ../../project/ch05
BOOT = timeout 3 $(QEMU) -m 128M -drive format=raw,file=build/$@/build/sprout.img \
       -display none -no-reboot

# $(call copy,NAME): a fresh copy of the project in build/NAME.
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

# The serial output, CRs removed. The kernel never exits: timeout stops it.
SERIAL = -serial stdio -monitor none 2>/dev/null | tr -d '\r' | sed -n '/^sprout 0.5/,$$p'

divzero:
	$(call copy,$@)
	sed -i '/back in kmain after int3/a\    { volatile int zero = 0; kprintf("%d\\n", 1 / zero); }' \
	    build/$@/kernel/kmain.c
	$(call build,$@)
	$(BOOT) $(SERIAL) | tee build/$@.log
	@echo "--- which source line is at the saved eip?"
	addr2line -f -i -s -e build/$@/build/kernel.elf \
	    $$(grep -o 'eip=[0-9a-f]*' build/$@.log | cut -d= -f2)

notpresent:
	$(call copy,$@)
	sed -i '/back in kmain after int3/a\    __asm__ volatile ("int $$0x80");' \
	    build/$@/kernel/kmain.c
	$(call build,$@)
	$(BOOT) $(SERIAL)

# After 3 seconds, ask the QEMU monitor about the PICs and the CPU.
noeoi:
	$(call copy,$@)
	sed -i '/pic_eoi(irq);/d' build/$@/kernel/interrupts.c
	$(call build,$@)
	(sleep 3; printf 'info pic\ninfo registers\nquit\n') | \
	    timeout 6 $(QEMU) -m 128M -drive format=raw,file=build/$@/build/sprout.img \
	    -display none -no-reboot -serial file:build/$@.log -monitor stdio \
	    | grep -E '^(pic[01]|EIP)' | tee build/$@.mon
	@echo "--- COM1:"; tr -d '\r' < build/$@.log | sed -n '/^sprout 0.5/,$$p'
	@echo "--- where is the CPU waiting?"
	addr2line -f -i -s -e build/$@/build/kernel.elf \
	    $$(grep -o 'EIP=[0-9a-f]*' build/$@.mon | cut -d= -f2)

noremap:
	$(call copy,$@)
	sed -i '/pic_init(IRQ_BASE);/,/IRQ_BASE + 15);/d' build/$@/kernel/kmain.c
	$(call build,$@)
	$(BOOT) $(SERIAL) | tee build/$@.log
	@echo "--- the \"error code\" is really an address:"
	addr2line -f -i -s -e build/$@/build/kernel.elf \
	    $$(grep -o 'error code [0-9a-f]*' build/$@.log | cut -d' ' -f3)

clean:
	rm -rf build

.PHONY: divzero notpresent noeoi noremap clean
