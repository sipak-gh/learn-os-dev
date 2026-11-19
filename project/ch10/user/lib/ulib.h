/* ulib.h — the C library of sprout's user programs: system call wrappers
 * and printf (Chapter 9). */
#ifndef SPROUT_ULIB_H
#define SPROUT_ULIB_H

#include <stddef.h>
#include <stdint.h>

/* System calls (kernel/syscall.h). On failure they return -errno. */
int write(int fd, const void *buf, size_t count);
void exit(int status) __attribute__((noreturn));
int getpid(void);
int sched_yield(void);

struct timespec {
    int32_t tv_sec;
    int32_t tv_nsec;
};
int nanosleep(const struct timespec *req, struct timespec *rem);

/* Built on the system calls. */
int sleep_ms(uint32_t ms);
int printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif
