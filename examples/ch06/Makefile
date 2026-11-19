# Makefile — Chapter 6 examples: what the keyboard and the terminal send.
#
# Each target copies project/ch06 to build/NAME, changes it (the diff is
# printed), builds it and boots it headless. Keys are pressed with the
# QEMU monitor's sendkey command; COM1 goes to build/NAME.log.
#
#   make scancodes  print every byte the keyboard IRQ reads from port 0x60
#   make set2       the same with the 8042's translation off: set 2 codes
#   make nocr       serial input without the CR -> '\n' translation
#   make clean

QEMU ?= qemu-system-i386
PROJECT := ../../project/ch06

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

# Boot, wait for the prompt, press: a, Shift+a, Enter, the up arrow.
KEYS := sendkey a\nsendkey shift-a\nsendkey ret\nsendkey up\n
# $(call press,NAME)
define press
	(sleep 2; printf '$(KEYS)'; sleep 1; printf 'quit\n') | \
	    timeout 8 $(QEMU) -m 128M -drive format=raw,file=build/$(1)/build/sprout.img \
	    -display none -no-reboot -serial file:build/$(1).log -monitor stdio >/dev/null
	@echo "--- COM1 after the prompt:"
	@tr -d '\r' < build/$(1).log | sed -n '/^type help/,$$p'; echo
endef

# Print each scancode in hex, before the driver looks at it.
SHOW_SCANCODE := sed -i -e 's/^\#include "keyboard.h"/&\n\#include "kprintf.h"/' \
    -e '/uint8_t sc = inb(KBD_DATA);/a\    kprintf("[%02x]", sc);'

scancodes:
	$(call copy,$@)
	$(SHOW_SCANCODE) build/$@/kernel/keyboard.c
	$(call build,$@)
	$(call press,$@)

set2:
	$(call copy,$@)
	cp set2.c build/$@/kernel/
	$(SHOW_SCANCODE) build/$@/kernel/keyboard.c
	sed -i -e '/^void keyboard_init(void)/,/^{/s/^{/{\n    kbd_translation_off();/' \
	    -e 's/^void keyboard_init(void)/void kbd_translation_off(void);\n\n&/' \
	    build/$@/kernel/keyboard.c
	$(call build,$@)
	$(call press,$@)

# Type "version" and Enter (CR) into COM1, as a terminal does.
nocr:
	$(call copy,$@)
	sed -i "/if (c == '\\\\r')/,+1d; s/        else if (c == 0x7F)/        if (c == 0x7F)/" \
	    build/$@/kernel/serial.c
	$(call build,$@)
	(sleep 2; printf 'version\r'; sleep 1) | timeout 5 $(QEMU) -m 128M \
	    -drive format=raw,file=build/$@/build/sprout.img -display none -no-reboot \
	    -serial stdio -monitor none 2>/dev/null | tr -d '\r' | sed -n '/^type help/,$$p'
	@echo; echo "(no reply: the shell is still waiting for '\\\\n')"

clean:
	rm -rf build

.PHONY: scancodes set2 nocr clean
