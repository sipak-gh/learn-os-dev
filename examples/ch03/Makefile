# Build every example boot sector into build/NAME.img (a 1 MiB disk image).
#
#   make                  build all of them
#   make run NAME=a20     boot one in a QEMU window (COM1 on this terminal)
#   make serial NAME=a20  boot one headless for 3 seconds, print COM1 output
#                         (QEMU_D="-d int,cpu_reset" adds QEMU's CPU event log)
#   make clean

NASM  ?= nasm
QEMU  ?= qemu-system-i386
NAMES := $(basename $(wildcard *.asm))

all: $(NAMES:%=build/%.img)

build/%.img: %.asm
	@mkdir -p build
	$(NASM) -f bin -w+all -w-reloc -o build/$*.bin $<
	cp build/$*.bin $@
	truncate -s 1M $@

run: build/$(NAME).img
	$(QEMU) -drive format=raw,file=$< -no-reboot -serial stdio

serial: build/$(NAME).img
	-timeout 3 $(QEMU) $(QEMU_D) -drive format=raw,file=$< -no-reboot \
	    -display none -serial stdio -monitor none

clean:
	rm -rf build

.PHONY: all run serial clean
