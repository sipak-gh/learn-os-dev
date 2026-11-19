/* input.c — one queue for every input device.
 *
 * Interrupt handlers fill the queue, the shell empties it. Waiting for
 * input must not miss a wake-up: if we checked "empty?" with interrupts
 * on and then ran hlt, a key arriving between the check and the hlt
 * would leave us asleep until the next one. So we check with interrupts
 * off, and use "sti; hlt": sti takes effect only after the following
 * instruction, so no interrupt can slip in between the two.
 */
#include "input.h"

#include "io.h"
#include "ringbuf.h"

static struct ringbuf queue;

void input_put(char c)
{
    ringbuf_put(&queue, c);
}

char input_getc(void)
{
    char c;

    for (;;) {
        cli();
        if (ringbuf_get(&queue, &c)) {
            sti();
            return c;
        }
        __asm__ volatile ("sti; hlt");  /* wait, with interrupts on */
    }
}
