/* programs.c — small kernel threads that show off the scheduler, and the
 * user programs from user/ (kernel/userbins.asm). */
#include "programs.h"

#include <stddef.h>
#include <stdint.h>

#include "kprintf.h"
#include "string.h"
#include "task.h"
#include "timer.h"
#include "user.h"

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

static const struct program kernel_programs[] = {
    { "ping", ping, NULL, 0 },
    { "pong", pong, NULL, 0 },
    { "tick", tick, NULL, 0 },
    { "tock", tock, NULL, 0 },
    { "hog",  hog,  NULL, 0 },
    { NULL, NULL, NULL, 0 },
};

extern const struct program user_programs[];    /* kernel/userbins.asm */

static const struct program *const tables[] = { kernel_programs, user_programs };

const struct program *program_find(const char *name)
{
    for (int i = 0; i < 2; i++)
        for (const struct program *p = tables[i]; p->name; p++)
            if (strcmp(p->name, name) == 0)
                return p;
    return NULL;
}

int program_start(const struct program *p)
{
    if (p->elf)
        return user_spawn(p->name, p->elf, p->size);
    int id = task_spawn(p->name, p->entry);
    if (id < 0)
        kprintf("%s: too many tasks\n", p->name);
    return id;
}

void program_list(void)
{
    const char *sep = "";
    for (int i = 0; i < 2; i++)
        for (const struct program *p = tables[i]; p->name; p++) {
            kprintf("%s%s", sep, p->name);
            sep = " ";
        }
}
