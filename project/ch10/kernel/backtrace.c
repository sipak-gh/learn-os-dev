/* backtrace.c — see backtrace.h. */
#include "backtrace.h"

#include <stdbool.h>
#include <stddef.h>

#include "interrupts.h"
#include "kprintf.h"
#include "ksyms.h"
#include "paging.h"

struct frame {
    const struct frame *next;       /* the caller's saved ebp */
    uint32_t ret;                   /* the return address */
};

extern const char isr_common[];     /* kernel/isr.asm */

static bool mapped(uint32_t addr)
{
    uint32_t cr0;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    if (!(cr0 & 0x80000000u))           /* a panic before paging is on */
        return addr < 0x100000u + 0x400000u;
    return paging_lookup_in(NULL, addr) & PAGE_PRESENT;
}

/* The chain may be garbage (that's often why we are panicking): only
 * follow pointers into mapped memory, and only upwards, so we can't fault
 * or go round in circles. */
static bool frame_ok(const struct frame *f, const struct frame *below)
{
    uint32_t a = (uint32_t)f;
    return a != 0 && (a & 3) == 0 && f > below
        && mapped(a) && mapped(a + sizeof *f - 1);
}

/* One line: the address, and function+offset. A return address is
 * looked up at addr - 1: the call may be a function's last instruction
 * (say, a call to a noreturn function), and then addr is already in the
 * next function. */
static uint32_t print_addr(uint32_t addr, bool is_return)
{
    uint32_t start;
    const char *name = ksyms_lookup(is_return ? addr - 1 : addr, &start);
    if (name)
        kprintf("  %08x  %s+0x%x\n", (unsigned)addr, name,
                (unsigned)(addr - start));
    else
        kprintf("  %08x  ?\n", (unsigned)addr);
    return name ? start : 0;
}

void backtrace_from(uint32_t ebp)
{
    const struct frame *f = (const struct frame *)ebp;
    const struct frame *below = NULL;

    kprintf("backtrace:\n");
    for (int depth = 0; depth < 32 && frame_ok(f, below); depth++) {
        if (f->ret == 0)                /* task_start's frame (task.c) */
            break;
        below = f;
        if (print_addr(f->ret, true) != (uint32_t)isr_common) {
            f = f->next;
            continue;
        }
        /* f is the frame of the function isr_common called, so its first
         * argument is a pointer to the interrupted code's registers
         * (interrupt_dispatch's, or the handler's that it tail-called:
         * same argument, same place). Carry on from there: the
         * instruction that was interrupted, then its caller. */
        const struct regs *r = ((const struct regs *const *)f)[2];
        if (!frame_ok((const struct frame *)r, below))
            break;
        kprintf("  -- interrupt %u --\n", (unsigned)r->vector);
        if (FROM_USER(r)) {
            kprintf("  %08x  (user mode)\n", (unsigned)r->eip);
            return;
        }
        print_addr(r->eip, false);
        f = (const struct frame *)r->ebp;
    }
}
