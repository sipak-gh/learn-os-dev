# Makefile — build sprout's disk image and boot it in QEMU.
#
#   make            build build/sprout.img (a 512-byte boot sector so far)
#   make run        boot it in a QEMU window; COM1 goes to this terminal
#   make run-nox    the same in this terminal (VGA via curses; quit: Esc 2, q)
#   make test       boot headless, compare COM1 output with tests/boot.expected
#   make demo       boot headless and show the COM1 output
#   make disasm     disassemble the boot sector as 16-bit code
#   make clean      delete build/

NASM  ?= nasm
QEMU  ?= qemu-system-i386
BUILD := build
IMAGE := $(BUILD)/sprout.img

# -f bin: raw bytes, no object file format. The BIOS runs bytes, not ELF.
# -w-reloc: -w+all would warn about every absolute address in a flat binary.
NASMFLAGS := -f bin -w+all -w-reloc

# Flags for every interactive run.
QEMUFLAGS := -drive format=raw,file=$(IMAGE) -no-reboot

all: $(IMAGE)

$(BUILD)/boot.bin: boot/boot.asm | $(BUILD)
	$(NASM) $(NASMFLAGS) -l $(BUILD)/boot.lst -o $@ $<

# The disk image: the boot sector, padded to 1 MiB (2048 sectors) so QEMU
# sees a small hard disk. Later chapters put more code after sector 0.
$(IMAGE): $(BUILD)/boot.bin
	cp $< $@
	truncate -s 1M $@

run: $(IMAGE)
	$(QEMU) $(QEMUFLAGS) -serial stdio

run-nox: $(IMAGE)
	$(QEMU) $(QEMUFLAGS) -display curses -serial file:$(BUILD)/serial.log

test: $(IMAGE)
	sh tools/qemu-test.sh "$(QEMU)" $(IMAGE) tests/boot.expected

demo: $(IMAGE)
	@sh tools/qemu-test.sh "$(QEMU)" $(IMAGE) tests/boot.expected >/dev/null; \
	    cat $(BUILD)/serial.log

disasm: $(BUILD)/boot.bin
	ndisasm -b 16 -o 0x7C00 $<

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)

.PHONY: all run run-nox test demo disasm clean
