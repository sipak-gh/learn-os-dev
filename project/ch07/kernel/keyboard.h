/* keyboard.h — the PS/2 keyboard on IRQ 1. */
#ifndef SPROUT_KEYBOARD_H
#define SPROUT_KEYBOARD_H

/* Start turning key presses into characters for input_getc(). */
void keyboard_init(void);

#endif
