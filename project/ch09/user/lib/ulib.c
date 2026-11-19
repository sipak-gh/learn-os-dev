/* ulib.c — see ulib.h. */
#include "ulib.h"

#include <stdarg.h>

#include "fmt.h"

/* Linux's i386 system call numbers (kernel/syscall.h). */
#define SYS_EXIT        1
#define SYS_WRITE       4
#define SYS_GETPID      20
#define SYS_SCHED_YIELD 158
#define SYS_NANOSLEEP   162

/* int 0x80 with the number in eax and up to three arguments in ebx, ecx
 * and edx; the result comes back in eax. "memory": the kernel may read
 * or write memory we pointed it at. */
static int syscall3(int n, uint32_t a, uint32_t b, uint32_t c)
{
    int result;
    __asm__ volatile ("int $0x80"
                      : "=a"(result)
                      : "a"(n), "b"(a), "c"(b), "d"(c)
                      : "memory");
    return result;
}

int write(int fd, const void *buf, size_t count)
{
    return syscall3(SYS_WRITE, fd, (uint32_t)buf, count);
}

void exit(int status)
{
    syscall3(SYS_EXIT, status, 0, 0);
    for (;;)
        ;                       /* exit doesn't return */
}

int getpid(void)
{
    return syscall3(SYS_GETPID, 0, 0, 0);
}

int sched_yield(void)
{
    return syscall3(SYS_SCHED_YIELD, 0, 0, 0);
}

int nanosleep(const struct timespec *req, struct timespec *rem)
{
    return syscall3(SYS_NANOSLEEP, (uint32_t)req, (uint32_t)rem, 0);
}

int sleep_ms(uint32_t ms)
{
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000 };
    return nanosleep(&ts, NULL);
}

/* Format into a buffer, then one write: the line comes out in one piece
 * even if other tasks print at the same time. */
int printf(const char *fmt, ...)
{
    char buf[256];
    va_list ap;

    va_start(ap, fmt);
    int n = fmt_vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n > (int)sizeof buf - 1)
        n = sizeof buf - 1;
    return write(1, buf, n);
}
