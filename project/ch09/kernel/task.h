/* task.h — kernel threads and a round-robin scheduler (Chapter 8).
 *
 * A task is a stack plus the registers switch_to saved on it. Kernel
 * tasks share the kernel's address space and run in ring 0; a user task
 * (Chapter 9) has its own address space and spends most of its time in
 * ring 3, entering the kernel on its kernel stack for system calls and
 * interrupts. Task 0, "idle", is the boot thread itself; it runs only
 * when no other task can.
 *
 *   READY ---- schedule() ---> RUNNING ---- returns or exits ---> ZOMBIE
 *     ^                         |    |                               |
 *     |   task_wake()           |    | time slice used up,           | task_wait()
 *     +--- SLEEPING/BLOCKED <---+    | task_yield()                  v
 *     +------------------------------+                             (gone)
 */
#ifndef SPROUT_TASK_H
#define SPROUT_TASK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TASK_MAX        16
#define TASK_STACK_SIZE 8192
#define TASK_SLICE      5       /* timer ticks a task may run before it is
                                   preempted: 50 ms at 100 Hz */
#define TASK_KILLED     (-1)    /* the exit status of a killed task */

enum task_state {
    TASK_READY,                 /* waiting for the CPU */
    TASK_RUNNING,               /* has the CPU (exactly one task) */
    TASK_SLEEPING,              /* until the timer reaches wake_at */
    TASK_BLOCKED,               /* until someone calls task_wake */
    TASK_ZOMBIE,                /* finished; waiting for task_wait */
};

struct task {
    int id;
    char name[12];
    enum task_state state;
    uint32_t esp;               /* saved by switch_to while not running */
    uint8_t *stack;             /* from kmalloc; NULL for idle */
    int (*entry)(void);
    uint32_t wake_at;           /* SLEEPING: timer tick to wake up at */
    int status;                 /* ZOMBIE: exit status */
    struct task *waiter;        /* the task blocked in task_wait for us */
    uint32_t *space;            /* page directory; NULL: the kernel's */
    uint32_t arg;               /* for entry: see task_spawn_in */
};

/* Turn the code that is running now into task 0, "idle". */
void task_init(void);

/* Start a task that runs entry() on its own stack; when entry returns,
 * its result is the exit status. Returns the new task's id, or -1. */
int task_spawn(const char *name, int (*entry)(void));

/* Like task_spawn, but the task runs in address space space (from
 * paging_new_space; freed with the task) and current->arg is arg. */
int task_spawn_in(const char *name, int (*entry)(void), uint32_t *space,
                  uint32_t arg);

struct task *task_current(void);

/* Give the CPU to the next ready task, if there is one. */
void task_yield(void);

/* Sleep for at least n timer ticks. */
void task_sleep(uint32_t n);

/* Make the current task BLOCKED and run something else until another task
 * or an interrupt handler calls task_wake on it. Call with interrupts
 * disabled, after checking the condition you are waiting for: otherwise
 * the wake-up could come between the check and the block (Chapter 6). */
void task_block(void);
void task_wake(struct task *t);

/* End the current task. */
void task_exit(int status) __attribute__((noreturn));

/* Wait until task id has ended, free it and store its exit status.
 * Returns false if there is no such task (or it is the caller). */
bool task_wait(int id, int *status);

/* Store task id's address space in *space (NULL: the kernel's). Returns
 * false for an unknown id. */
bool task_space(int id, uint32_t **space);

/* Copy task id's name into buf (size n). Returns false for an unknown id. */
bool task_name(int id, char *buf, size_t n);

/* Stop task id; it becomes a zombie with status TASK_KILLED. Returns false
 * for an unknown id, idle, the caller, or a task that has already ended. */
bool task_kill(int id);

/* Called by the timer interrupt handler on every tick. */
void task_tick(uint32_t now);

/* Called at the end of every IRQ: switch tasks if task_tick or task_wake
 * asked for it. */
void task_preempt(void);

/* The ps command. */
void task_print(void);

#endif
