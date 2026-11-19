/* input.h — characters typed on the keyboard or received on COM1. */
#ifndef SPROUT_INPUT_H
#define SPROUT_INPUT_H

/* Called by interrupt handlers: queue one character (dropped if full). */
void input_put(char c);

/* Wait for the next character; other tasks run meanwhile. Only one task
 * may read input at a time. */
char input_getc(void);

#endif
