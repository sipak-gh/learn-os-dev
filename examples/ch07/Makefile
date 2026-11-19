# Makefile — Chapter 7 examples: memory, before and after paging.
#
# Each target copies a project snapshot to build/NAME, changes one thing
# (the diff is printed), builds it and boots it headless.
#
#   make null06     Chapter 6's kernel reads address 0: no fault, just data
#   make bootinfo   using boot_info after paging: a page fault at 0x500
#   make nowp       no CR0.WP: the kernel writes to its read-only code
#   make noinvlpg   remap a page without invlpg: the TLB remembers
#   make clean

QEMU ?= qemu-system-i386
PROJECT := ../../project/ch07
IMG = -drive format=raw,file=build/$@/build/sprout.img

# $(call copy,NAME[,SNAPSHOT]): a fresh copy of a snapshot in build/NAME.
define copy
	rm -rf build/$(1) && mkdir -p build
	cp -r $(or $(2),$(PROJECT)) build/$(1)
	rm -rf build/$(1)/build
endef

# $(call build,NAME[,SNAPSHOT]): show what changed, then build quietly.
define build
	diff -ru -x build $(or $(2),$(PROJECT)) build/$(1) || true
	$(MAKE) -s -C build/$(1)
endef

# $(call type,TEXT): boot, type TEXT into COM1 once the shell is up, and
# show COM1 from the version line on.
define type
	(sleep 2; printf '$(1)'; sleep 1) | timeout 5 $(QEMU) -m 128M $(IMG) \
	    -display none -no-reboot -serial stdio -monitor none 2>/dev/null \
	    | tr -d '\r' | sed -n '/^sprout 0/,$$p'
endef

null06:
	$(call copy,$@,../../project/ch06)
	sed -i '/^    memmap_print(bi);/a\    {\n        uint32_t *volatile p = 0;\n        kprintf("*(uint32_t *)0 = %08x\\n", (unsigned)*p);\n    }' \
	    build/$@/kernel/kmain.c
	$(call build,$@,../../project/ch06)
	timeout 3 $(QEMU) -m 128M $(IMG) -display none -no-reboot \
	    -serial stdio -monitor none 2>/dev/null | tr -d '\r' | grep -A1 '^usable RAM'
	@echo "--- real-mode IVT entry 0 (divide error): f000:ff53, a lone iret in the BIOS"

bootinfo:
	$(call copy,$@)
	sed -i '/^    kheap_init();/i\    kprintf("boot drive: %x\\n", (unsigned)bi->boot_drive);' \
	    build/$@/kernel/kmain.c
	$(call build,$@)
	timeout 3 $(QEMU) -m 128M $(IMG) -display none -no-reboot \
	    -serial stdio -monitor none 2>/dev/null | tr -d '\r' | sed -n '/^paging/,$$p'

nowp:
	$(call copy,$@)
	sed -i 's/ | CR0_PG | CR0_WP);/ | CR0_PG);/' build/$@/kernel/paging.c
	$(call build,$@)
	$(call type,crash code\rvm\r) | sed -n '/^sprout> crash/,$$p'

noinvlpg:
	$(call copy,$@)
	sed -i '/^    if (enabled)$$/{N;/invlpg(virt);/d}' build/$@/kernel/paging.c
	sed -i '/^    kheap_init();/i\    {\n        uint32_t a = pmm_alloc(), b = pmm_alloc();\n        *(uint32_t *)a = 0xAAAAAAAA;\n        *(uint32_t *)b = 0xBBBBBBBB;\n        paging_map(0xE0000000, a, PAGE_WRITE);\n        kprintf("e0000000 -> %08x: %08x\\n", (unsigned)a, *(volatile uint32_t *)0xE0000000);\n        paging_map(0xE0000000, b, PAGE_WRITE);\n        kprintf("e0000000 -> %08x: %08x\\n", (unsigned)b, *(volatile uint32_t *)0xE0000000);\n    }' \
	    build/$@/kernel/kmain.c
	$(call build,$@)
	timeout 3 $(QEMU) -m 128M $(IMG) -display none -no-reboot \
	    -serial stdio -monitor none 2>/dev/null | tr -d '\r' | grep '^e0000000'

clean:
	rm -rf build

.PHONY: null06 bootinfo nowp noinvlpg clean
