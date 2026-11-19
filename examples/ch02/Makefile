# Each example is a boot sector NAME.asm. The disk image is the boot sector,
# then payload.asm (sector 1: a message to load), padded to 1 MiB.
#
#   make                  build build/NAME.img for every example
#   make serial NAME=chs  boot one headless for 3 seconds, print COM1 output
#   make clean

NASM  ?= nasm
QEMU  ?= qemu-system-i386
NAMES := $(filter-out payload,$(basename $(wildcard *.asm)))

all: $(NAMES:%=build/%.img)

build/%.bin: %.asm
	@mkdir -p build
	$(NASM) -f bin -w+all -w-reloc -o $@ $<

build/%.img: build/%.bin build/payload.bin
	cat $^ > $@
	truncate -s 1M $@

serial: build/$(NAME).img
	-timeout 3 $(QEMU) -drive format=raw,file=$< -no-reboot \
	    -display none -serial stdio -monitor none

clean:
	rm -rf build

.PHONY: all serial clean
