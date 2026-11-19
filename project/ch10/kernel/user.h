/* user.h — user programs: ring 3, their own address space (Chapter 9). */
#ifndef SPROUT_USER_H
#define SPROUT_USER_H

#include <stdbool.h>
#include <stdint.h>

#include "interrupts.h"

/* Every user program gets USER_STACK_SIZE bytes of stack just below
 * USER_TOP. */
#define USER_STACK_SIZE (16u * 1024)

/* Start the ELF executable file[0..size) as a new user task. Returns the
 * task's id, or -1 after printing why not. */
int user_spawn(const char *name, const uint8_t *file, uint32_t size);

/* Is [p, p + n) user memory the current task may read (or, with write,
 * write)? System calls check every pointer they get from user code. */
bool user_access_ok(const void *p, uint32_t n, bool write);

/* The current task, a user program, caused the exception in r: report it
 * and end the task. */
void user_kill(const struct regs *r) __attribute__((noreturn));

#endif
