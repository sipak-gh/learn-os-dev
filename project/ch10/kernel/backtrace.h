/* backtrace.h — who called whom: walking the frame pointer chain
 * (Chapter 10).
 *
 * The kernel is compiled with -fno-omit-frame-pointer, so every function
 * starts with push ebp; mov ebp, esp. Each frame then begins with the
 * caller's saved ebp and, above it, the return address:
 *
 *     ebp + 4:  return address (into the caller)
 *     ebp + 0:  the caller's ebp  ---> the caller's frame, and so on
 *
 * The chain ends at ebp = 0, which entry.asm and task_spawn put at the
 * bottom of every stack, or at a return address of 0 (task_spawn again).
 */
#ifndef SPROUT_BACKTRACE_H
#define SPROUT_BACKTRACE_H

#include <stdint.h>

/* Print the return addresses (with function names) of the frames from
 * ebp up. Interrupt frames are followed into the interrupted code. */
void backtrace_from(uint32_t ebp);

/* The backtrace of whoever calls this. */
#define backtrace() backtrace_from((uint32_t)__builtin_frame_address(0))

#endif
