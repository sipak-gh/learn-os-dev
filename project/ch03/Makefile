# Makefile — build sprout's disk image and boot it in QEMU.
#
#   make            build build/sprout.img
#   make run        boot it in a QEMU window; COM1 goes to this terminal
#   make run-nox    the same in this terminal (VGA via curses; quit: Esc 2, q)
#   make test       boot headless, compare COM1 output with tests/boot.expected
#   make demo       boot headless and show the COM1 output
#   make disasm     disassemble the boot sector and stage 2 as 16-bit code
#   make clean      delete build/
#
# Disk image layout (512-byte sectors):
#   LBA 0           boot sector (boot/boot.asm)
#   LBA 1..16       stage 2 (boot/stage2.asm), padded to 8 KiB

NASM  ?= nasm
QEMU  ?= qemu-system-i386
BUILD := build
IMAGE := $(BUILD)/sprout.img

# -f bin: raw bytes, no object file format. The BIOS runs bytes, not ELF.
# -w-reloc: -w+all would warn about every absolute address in a flat binary.
# -I boot/: where %include looks.
NASMFLAGS := -f bin -w+all -w-reloc -I boot/

# 128 MiB of RAM, always: the memory map in the test output depends on it.
QEMUFLAGS := -m 128M -drive format=raw,file=$(IMAGE) -no-reboot

all: $(IMAGE)

# -MD/-MP: record which .inc files each .asm used (build/*.d, included
# at the bottom), so editing layout.inc rebuilds both binaries.
$(BUILD)/%.bin: boot/%.asm | $(BUILD)
	$(NASM) $(NASMFLAGS) -MD $(@:.bin=.d) -MP -l $(@:.bin=.lst) -o $@ $<

# The disk image: boot sector, then stage 2, padded to 1 MiB (2048
# sectors) so QEMU sees a small hard disk.
$(IMAGE): $(BUILD)/boot.bin $(BUILD)/stage2.bin
	cat $^ > $@
	truncate -s 1M $@

run: $(IMAGE)
	$(QEMU) $(QEMUFLAGS) -serial stdio

run-nox: $(IMAGE)
	$(QEMU) $(QEMUFLAGS) -display curses -serial file:$(BUILD)/serial.log

test: $(IMAGE)
	sh tools/qemu-test.sh "$(QEMU) -m 128M" $(IMAGE) tests/boot.expected

demo: $(IMAGE)
	@sh tools/qemu-test.sh "$(QEMU) -m 128M" $(IMAGE) tests/boot.expected >/dev/null; \
	    cat $(BUILD)/serial.log

disasm: $(BUILD)/boot.bin $(BUILD)/stage2.bin
	ndisasm -b 16 -o 0x7C00 $(BUILD)/boot.bin
	ndisasm -b 16 -o 0x8000 $(BUILD)/stage2.bin

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)

.PHONY: all run run-nox test demo disasm clean

-include $(wildcard $(BUILD)/*.d)
