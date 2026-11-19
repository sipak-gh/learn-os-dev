/* input.c — one queue for every input device.
 *
 * Interrupt handlers fill the queue, a task (the shell) empties it. A task
 * that finds the queue empty blocks, and the CPU runs other tasks until a
 * character arrives. The wake-up must not get lost: if we checked "empty?"
 * with interrupts on, a key could arrive between the check and
 * task_block, and nobody would wake us. So we check and block with
 * interrupts off; schedule() switches to a task that turns them back on
 * (Chapter 6 did the same with "sti; hlt").
 */
#include "input.h"

#include "io.h"
#include "ringbuf.h"
#include "task.h"

static struct ringbuf queue;
static struct task *waiting;        /* the task blocked in input_getc */

void input_put(char c)
{
    ringbuf_put(&queue, c);
    if (waiting) {
        task_wake(waiting);
        waiting = NULL;
    }
}

char input_getc(void)
{
    char c;
    uint32_t flags = irq_save();

    while (!ringbuf_get(&queue, &c)) {
        waiting = task_current();
        task_block();
    }
    irq_restore(flags);
    return c;
}
