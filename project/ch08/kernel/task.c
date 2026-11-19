/* task.c — see task.h. */
#include "task.h"

#include "io.h"
#include "kheap.h"
#include "kprintf.h"
#include "string.h"
#include "timer.h"

/* kernel/switch.asm */
void switch_to(uint32_t *save_esp, uint32_t new_esp);

static struct task idle_task;
static struct task *tasks[TASK_MAX];    /* slot 0 is idle */
static struct task *current;
static int next_id;
static uint32_t slice;                  /* ticks since the last switch */
static bool need_resched;

static const char *const state_names[] = {
    [TASK_READY] = "ready",       [TASK_RUNNING] = "running",
    [TASK_SLEEPING] = "sleeping", [TASK_BLOCKED] = "blocked",
    [TASK_ZOMBIE] = "zombie",
};

static void set_name(struct task *t, const char *name)
{
    size_t n = strlen(name);
    if (n >= sizeof t->name)
        n = sizeof t->name - 1;
    memcpy(t->name, name, n);
    t->name[n] = '\0';
}

static int slot_of(const struct task *t)
{
    for (int i = 0; i < TASK_MAX; i++)
        if (tasks[i] == t)
            return i;
    panic("task %d is not in the table", t->id);
}

static struct task *find(int id)
{
    for (int i = 0; i < TASK_MAX; i++)
        if (tasks[i] && tasks[i]->id == id)
            return tasks[i];
    return NULL;
}

/* Round robin: the first runnable task after the current one in the
 * table, wrapping around; the current task itself comes last. Idle only
 * if there is nothing else. */
static struct task *pick_next(void)
{
    int start = slot_of(current);

    for (int i = 1; i <= TASK_MAX; i++) {
        struct task *t = tasks[(start + i) % TASK_MAX];
        if (t && t != &idle_task
            && (t->state == TASK_READY || t->state == TASK_RUNNING))
            return t;
    }
    return &idle_task;
}

/* Run the next task. Interrupts must be off: an interrupt handler that
 * switched tasks in the middle of this would make a mess of current. */
static void schedule(void)
{
    struct task *prev = current;
    struct task *next = pick_next();

    need_resched = false;
    slice = 0;
    if (next == prev)
        return;
    if (prev->state == TASK_RUNNING)
        prev->state = TASK_READY;
    next->state = TASK_RUNNING;
    current = next;
    switch_to(&prev->esp, next->esp);
    /* Back again: some other task has switched to us. */
}

void task_init(void)
{
    set_name(&idle_task, "idle");
    idle_task.id = next_id++;
    idle_task.state = TASK_RUNNING;
    tasks[0] = &idle_task;
    current = &idle_task;
}

/* A new task's first switch_to "returns" here, with interrupts still
 * disabled by whoever called schedule(). */
static void task_start(void)
{
    sti();
    task_exit(current->entry());
}

int task_spawn(const char *name, int (*entry)(void))
{
    struct task *t = kmalloc(sizeof *t);
    uint8_t *stack = kmalloc(TASK_STACK_SIZE);
    if (!t || !stack) {
        kfree(t);
        kfree(stack);
        return -1;
    }
    memset(t, 0, sizeof *t);
    set_name(t, name);
    t->stack = stack;
    t->entry = entry;
    t->state = TASK_READY;

    /* Make the stack look as if the task had called switch_to itself:
     * four saved registers, then a return address. */
    uint32_t *sp = (uint32_t *)(stack + TASK_STACK_SIZE);
    *--sp = 0;                          /* task_start's return address */
    *--sp = (uint32_t)task_start;       /* where switch_to's ret goes */
    *--sp = 0;                          /* ebp = 0 ends backtraces */
    *--sp = 0;                          /* ebx */
    *--sp = 0;                          /* esi */
    *--sp = 0;                          /* edi */
    t->esp = (uint32_t)sp;

    uint32_t flags = irq_save();
    int slot = 0;
    while (slot < TASK_MAX && tasks[slot])
        slot++;
    if (slot == TASK_MAX) {
        irq_restore(flags);
        kfree(stack);
        kfree(t);
        return -1;
    }
    t->id = next_id++;
    tasks[slot] = t;
    irq_restore(flags);
    return t->id;
}

struct task *task_current(void)
{
    return current;
}

void task_yield(void)
{
    uint32_t flags = irq_save();
    schedule();
    irq_restore(flags);
}

void task_sleep(uint32_t n)
{
    uint32_t flags = irq_save();
    current->wake_at = timer_ticks() + n;
    current->state = TASK_SLEEPING;
    schedule();
    irq_restore(flags);
}

void task_block(void)
{
    current->state = TASK_BLOCKED;
    schedule();
}

void task_wake(struct task *t)
{
    if (t->state == TASK_BLOCKED || t->state == TASK_SLEEPING) {
        t->state = TASK_READY;
        need_resched = true;    /* let it run soon, not after a whole slice */
    }
}

void task_exit(int status)
{
    irq_save();
    current->state = TASK_ZOMBIE;
    current->status = status;
    if (current->waiter)
        task_wake(current->waiter);
    schedule();
    panic("a zombie ran");      /* nobody switches to a zombie */
}

/* Remove t from the table and free it. It must not be running. */
static void reap(struct task *t)
{
    tasks[slot_of(t)] = NULL;
    for (int i = 0; i < TASK_MAX; i++)
        if (tasks[i] && tasks[i]->waiter == t)
            tasks[i]->waiter = NULL;
    kfree(t->stack);
    kfree(t);
}

bool task_wait(int id, int *status)
{
    uint32_t flags = irq_save();
    struct task *t = find(id);

    if (!t || t == current || t == &idle_task
        || (t->waiter && t->waiter != current)) {
        irq_restore(flags);
        return false;
    }
    while (t->state != TASK_ZOMBIE) {
        t->waiter = current;
        task_block();           /* task_exit or task_kill wakes us */
    }
    *status = t->status;
    reap(t);
    irq_restore(flags);
    return true;
}

bool task_name(int id, char *buf, size_t n)
{
    uint32_t flags = irq_save();
    struct task *t = find(id);
    if (t && n > 0) {
        size_t len = strlen(t->name);
        if (len >= n)
            len = n - 1;
        memcpy(buf, t->name, len);
        buf[len] = '\0';
    }
    irq_restore(flags);
    return t != NULL;
}

bool task_kill(int id)
{
    uint32_t flags = irq_save();
    struct task *t = find(id);
    bool ok = t && t != &idle_task && t != current && t->state != TASK_ZOMBIE;

    if (ok) {
        /* It is not running (we are), so it is stopped somewhere inside
         * schedule(); it just never gets picked again. */
        t->state = TASK_ZOMBIE;
        t->status = TASK_KILLED;
        if (t->waiter)
            task_wake(t->waiter);
    }
    irq_restore(flags);
    return ok;
}

void task_tick(uint32_t now)
{
    if (!current)
        return;                 /* the timer runs before task_init */
    for (int i = 0; i < TASK_MAX; i++) {
        struct task *t = tasks[i];
        if (t && t->state == TASK_SLEEPING && (int32_t)(now - t->wake_at) >= 0)
            task_wake(t);
    }
    if (++slice >= TASK_SLICE)
        need_resched = true;
}

void task_preempt(void)
{
    if (current && need_resched)
        schedule();
}

void task_print(void)
{
    uint32_t flags = irq_save();

    kprintf("  ID  STATE     STACK              NAME\n");
    for (int i = 0; i < TASK_MAX; i++) {
        struct task *t = tasks[i];
        if (!t)
            continue;
        kprintf("  %2d  %-8s  ", t->id, state_names[t->state]);
        if (t->stack)
            kprintf("%08x-%08x", (unsigned)t->stack,
                    (unsigned)(t->stack + TASK_STACK_SIZE - 1));
        else
            kprintf("%-17s", "boot stack");
        kprintf("  %s\n", t->name);
    }
    irq_restore(flags);
}
