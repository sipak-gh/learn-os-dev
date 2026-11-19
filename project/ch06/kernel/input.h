/* input.h — characters typed on the keyboard or received on COM1. */
#ifndef SPROUT_INPUT_H
#define SPROUT_INPUT_H

/* Called by interrupt handlers: queue one character (dropped if full). */
void input_put(char c);

/* Wait for the next character. Needs interrupts enabled. */
char input_getc(void);

#endif
