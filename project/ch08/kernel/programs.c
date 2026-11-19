/* programs.c — small kernel threads that show off the scheduler. */
#include "programs.h"

#include <stddef.h>
#include <stdint.h>

#include "kprintf.h"
#include "string.h"
#include "task.h"
#include "timer.h"

/* ping and pong take turns by yielding: cooperative multitasking. */
static int ping(void)
{
    for (int i = 1; i <= 3; i++) {
        kprintf("ping %d\n", i);
        task_yield();
    }
    return 0;
}

static int pong(void)
{
    for (int i = 1; i <= 3; i++) {
        kprintf("pong %d\n", i);
        task_yield();
    }
    return 0;
}

/* tick and tock sleep; the timer decides who runs next. */
static int tick(void)
{
    for (int i = 1; i <= 4; i++) {
        task_sleep(timer_ms_to_ticks(100));
        kprintf("tick %d\n", i);
    }
    return 0;
}

static int tock(void)
{
    for (int i = 1; i <= 2; i++) {
        task_sleep(timer_ms_to_ticks(170));
        kprintf("tock %d\n", i);
    }
    return 0;
}

/* hog never yields or sleeps: only preemption gets the CPU back. */
static volatile uint32_t hog_loops;

__attribute__((noreturn)) static int hog(void)
{
    for (;;)
        hog_loops++;
}

static const struct program programs[] = {
    { "ping", ping },
    { "pong", pong },
    { "tick", tick },
    { "tock", tock },
    { "hog",  hog },
    { NULL, NULL },
};

const struct program *program_find(const char *name)
{
    for (const struct program *p = programs; p->name; p++)
        if (strcmp(p->name, name) == 0)
            return p;
    return NULL;
}

void program_list(void)
{
    for (const struct program *p = programs; p->name; p++)
        kprintf("%s%s", p->name, p[1].name ? " " : "");
}
