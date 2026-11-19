/* syscall.h — system calls: int 0x80 from user mode (Chapter 9).
 *
 * The same convention as 32-bit Linux: the call number in eax, arguments
 * in ebx, ecx, edx, the result in eax. A negative result is an error
 * number (-14 is EFAULT, "bad address"). The numbers are Linux's too, so
 * a 32-bit Linux program that only uses these calls runs on sprout.
 */
#ifndef SPROUT_SYSCALL_H
#define SPROUT_SYSCALL_H

#define SYS_EXIT        1       /* exit(status) */
#define SYS_WRITE       4       /* write(fd, buf, count): fd 1 or 2 */
#define SYS_GETPID      20      /* getpid(): the task id */
#define SYS_SCHED_YIELD 158     /* sched_yield() */
#define SYS_NANOSLEEP   162     /* nanosleep(&timespec, rem): rem ignored */

#define EBADF   9
#define EFAULT  14
#define EINVAL  22
#define ENOSYS  38

/* Install the int 0x80 handler. */
void syscall_init(void);

#endif
