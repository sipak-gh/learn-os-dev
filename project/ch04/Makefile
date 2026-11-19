# Makefile — build sprout's disk image and boot it in QEMU.
#
#   make            build build/sprout.img
#   make run        boot it in a QEMU window; COM1 goes to this terminal
#   make run-nox    the same in this terminal (VGA via curses; quit: Esc 2, q)
#   make test       run the unit tests, then boot headless and compare
#                   COM1 output with tests/boot.expected
#   make unit       just the unit tests (lib/ compiled for this machine)
#   make demo       boot headless and show the COM1 output
#   make disasm     disassemble the boot sector, stage 2 and the kernel
#   make clean      delete build/
#
# Disk image layout (512-byte sectors):
#   LBA 0           boot sector (boot/boot.asm)
#   LBA 1..16       stage 2 (boot/stage2.asm), padded to 8 KiB
#   LBA 17..        the kernel (kernel/, lib/), a flat binary linked for 1 MiB

NASM    ?= nasm
QEMU    ?= qemu-system-i386
CC      := gcc
LD      := ld
OBJCOPY := objcopy
HOSTCC  ?= cc
BUILD   := build
IMAGE   := $(BUILD)/sprout.img

# -f bin: raw bytes, no object file format. The BIOS runs bytes, not ELF.
# -w-reloc: -w+all would warn about every absolute address in a flat binary.
# -I boot/: where %include looks.
NASMFLAGS := -f bin -w+all -w-reloc -I boot/

# The kernel is 32-bit code with no C library and no operating system
# under it, built by the ordinary 64-bit gcc:
#   -m32                      i386 code (and -m elf_i386 for ld)
#   -ffreestanding            no hosted C library: main() isn't special,
#                             printf isn't printf
#   -fno-pie                  absolute addresses; there is no loader to
#                             relocate a position-independent kernel
#   -fno-stack-protector      no stack canaries (they read %gs:0x14 and
#                             call __stack_chk_fail); many distributions'
#                             gcc turn them on by default
#   -fcf-protection=none      no endbr32 instructions (same story)
#   -fno-asynchronous-unwind-tables   no .eh_frame: nobody unwinds
#   -mgeneral-regs-only       no SSE/x87 code: we never save those registers
# No -lgcc: this machine has no 32-bit libgcc, so any C that needs a helper
# from it (64-bit division, say) fails to link. lib/fmt.c works around it.
KCFLAGS := -m32 -ffreestanding -fno-pie -fno-stack-protector \
           -fcf-protection=none -fno-asynchronous-unwind-tables \
           -mgeneral-regs-only -std=c11 -O2 -g -Wall -Wextra -Werror \
           -Ikernel -Ilib -MMD -MP
KLDFLAGS := -m elf_i386 -nostdlib -T kernel/linker.ld

KSRCS := $(wildcard kernel/*.c lib/*.c)
KOBJS := $(BUILD)/kernel/entry.o $(KSRCS:%.c=$(BUILD)/%.o)

# The unit tests run lib/ on this machine, against tests/unit/.
# -fno-builtin: call our memcpy and strlen, not gcc's built-in versions.
# -iquote lib: "string.h" is ours; <string.h> is still the C library's.
HOSTCFLAGS := -std=c11 -g -O1 -Wall -Wextra -Werror -fno-builtin \
              -iquote lib -MMD -MP
USRCS := $(wildcard tests/unit/*.c lib/*.c)
UOBJS := $(USRCS:%.c=$(BUILD)/host/%.o)

# 128 MiB of RAM, always: the memory map in the test output depends on it.
QEMUFLAGS := -m 128M -drive format=raw,file=$(IMAGE) -no-reboot

all: $(IMAGE)

# --- the boot sector and stage 2 -----------------------------------------

$(BUILD)/boot.bin: boot/boot.asm | $(BUILD)
	$(NASM) $(NASMFLAGS) -MD $(@:.bin=.d) -MP -l $(@:.bin=.lst) -o $@ $<

# Stage 2 must know how many sectors to load, so it is assembled after
# the kernel, with KERNEL_SECTORS = the kernel's size rounded up.
$(BUILD)/stage2.bin: boot/stage2.asm $(BUILD)/kernel.bin | $(BUILD)
	@echo "kernel.bin: $$(stat -c %s $(BUILD)/kernel.bin) bytes," \
	    "$$(( ($$(stat -c %s $(BUILD)/kernel.bin) + 511) / 512 )) sectors"
	$(NASM) $(NASMFLAGS) -MD $(@:.bin=.d) -MP -l $(@:.bin=.lst) \
	    -DKERNEL_SECTORS=$$(( ($$(stat -c %s $(BUILD)/kernel.bin) + 511) / 512 )) \
	    -o $@ $<

# --- the kernel -----------------------------------------------------------------

$(BUILD)/kernel/entry.o: kernel/entry.asm
	@mkdir -p $(@D)
	$(NASM) -f elf32 -g -F dwarf -w+all -w-reloc -MD $(@:.o=.d) -MP -o $@ $<

$(BUILD)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(KCFLAGS) -c -o $@ $<

# kernel.elf keeps symbols and debug info (for objdump, nm and gdb);
# kernel.bin is the bytes stage 2 copies to 1 MiB.
$(BUILD)/kernel.elf: $(KOBJS) kernel/linker.ld
	$(LD) $(KLDFLAGS) -o $@ $(KOBJS)

$(BUILD)/kernel.bin: $(BUILD)/kernel.elf
	$(OBJCOPY) -O binary $< $@

# --- the disk image -------------------------------------------------------------

# Boot sector, stage 2, kernel, padded to 1 MiB (2048 sectors) so QEMU
# sees a small hard disk.
$(IMAGE): $(BUILD)/boot.bin $(BUILD)/stage2.bin $(BUILD)/kernel.bin
	cat $^ > $@
	truncate -s 1M $@

# --- unit tests -------------------------------------------------------------------

$(BUILD)/host/%.o: %.c
	@mkdir -p $(@D)
	$(HOSTCC) $(HOSTCFLAGS) -c -o $@ $<

$(BUILD)/unit: $(UOBJS)
	$(HOSTCC) -o $@ $^

unit: $(BUILD)/unit
	$(BUILD)/unit

# --- running ------------------------------------------------------------------------

run: $(IMAGE)
	$(QEMU) $(QEMUFLAGS) -serial stdio

run-nox: $(IMAGE)
	$(QEMU) $(QEMUFLAGS) -display curses -serial file:$(BUILD)/serial.log

test: unit $(IMAGE)
	sh tools/qemu-test.sh "$(QEMU) -m 128M" $(IMAGE) tests/boot.expected

demo: $(IMAGE)
	@sh tools/qemu-test.sh "$(QEMU) -m 128M" $(IMAGE) tests/boot.expected >/dev/null; \
	    cat $(BUILD)/serial.log

disasm: $(BUILD)/boot.bin $(BUILD)/stage2.bin $(BUILD)/kernel.elf
	ndisasm -b 16 -o 0x7C00 $(BUILD)/boot.bin
	ndisasm -b 16 -o 0x8000 $(BUILD)/stage2.bin
	objdump -d -M intel --no-show-raw-insn $(BUILD)/kernel.elf

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)

.PHONY: all unit run run-nox test demo disasm clean

-include $(wildcard $(BUILD)/*.d $(BUILD)/*/*.d $(BUILD)/*/*/*.d)
