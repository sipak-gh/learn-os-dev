/* syscall.c — see syscall.h. */
#include "syscall.h"

#include "interrupts.h"
#include "io.h"
#include "kprintf.h"
#include "task.h"
#include "timer.h"
#include "user.h"

/* Linux's struct timespec on i386. */
struct timespec {
    int32_t tv_sec;
    int32_t tv_nsec;
};

static int32_t sys_exit(uint32_t status)
{
    task_exit((int)status);
}

/* buf is a user pointer: check it before touching it. Without the check,
 * a program could make the kernel print kernel memory, or fault on an
 * unmapped address in ring 0, which is a kernel panic. */
static int32_t sys_write(uint32_t fd, uint32_t buf, uint32_t count)
{
    if (fd != 1 && fd != 2)
        return -EBADF;
    if (!user_access_ok((const void *)buf, count, false))
        return -EFAULT;
    const char *s = (const char *)buf;
    uint32_t flags = irq_save();        /* one write, one piece */
    for (uint32_t i = 0; i < count; i++)
        console_putc(s[i]);
    irq_restore(flags);
    return (int32_t)count;
}

static int32_t sys_nanosleep(uint32_t req)
{
    if (!user_access_ok((const void *)req, sizeof(struct timespec), false))
        return -EFAULT;
    const struct timespec *ts = (const struct timespec *)req;
    if (ts->tv_sec < 0 || ts->tv_nsec < 0 || ts->tv_nsec > 999999999)
        return -EINVAL;
    uint32_t ms = (uint32_t)ts->tv_sec * 1000 + (uint32_t)ts->tv_nsec / 1000000;
    task_sleep(timer_ms_to_ticks(ms));
    return 0;
}

/* The int 0x80 handler. The program's registers are in r: read the
 * arguments from there, and put the result in r->eax, which isr_common
 * restores before iret. */
static void syscall_handler(struct regs *r)
{
    int32_t result;

    switch (r->eax) {
    case SYS_EXIT:        result = sys_exit(r->ebx); break;
    case SYS_WRITE:       result = sys_write(r->ebx, r->ecx, r->edx); break;
    case SYS_GETPID:      result = task_current()->id; break;
    case SYS_SCHED_YIELD: task_yield(); result = 0; break;
    case SYS_NANOSLEEP:   result = sys_nanosleep(r->ebx); break;
    default:              result = -ENOSYS; break;
    }
    r->eax = (uint32_t)result;
}

void syscall_init(void)
{
    interrupt_register(VECTOR_SYSCALL, syscall_handler);
}
