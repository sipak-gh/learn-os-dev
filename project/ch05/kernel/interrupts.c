/* interrupts.c — route each interrupt to its handler; report exceptions. */
#include "interrupts.h"

#include <stddef.h>

#include "kprintf.h"
#include "pic.h"

static interrupt_handler handlers[IRQ_BASE + 16];

/* Intel SDM Vol. 3A, table 6-1. */
static const char *const exception_names[32] = {
    "divide error", "debug", "NMI", "breakpoint", "overflow",
    "BOUND range exceeded", "invalid opcode", "device not available",
    "double fault", "coprocessor segment overrun", "invalid TSS",
    "segment not present", "stack-segment fault", "general protection",
    "page fault", "reserved", "x87 floating-point error", "alignment check",
    "machine check", "SIMD floating-point exception", "virtualization",
    "control protection",
};

void interrupt_register(uint8_t vector, interrupt_handler handler)
{
    if (vector < sizeof handlers / sizeof handlers[0])
        handlers[vector] = handler;
}

/* esp_pusha is where esp pointed when pusha ran: at the vector number.
 * The interrupted code's esp was 5 words higher (vector, error code, eip,
 * cs, eflags). That holds for interrupts taken in ring 0 only. */
void regs_dump(const struct regs *r)
{
    kprintf("  eax=%08x ebx=%08x ecx=%08x edx=%08x\n",
            r->eax, r->ebx, r->ecx, r->edx);
    kprintf("  esi=%08x edi=%08x ebp=%08x esp=%08x\n",
            r->esi, r->edi, r->ebp, r->esp_pusha + 20);
    kprintf("  eip=%08x cs=%04x eflags=%08x\n", r->eip, r->cs, r->eflags);
    kprintf("  ds=%04x es=%04x fs=%04x gs=%04x\n", r->ds, r->es, r->fs, r->gs);
}

static void unhandled_exception(struct regs *r)
{
    const char *name = r->vector < 22 ? exception_names[r->vector] : "reserved";
    kprintf("\nexception %u (%s), error code %x\n",
            r->vector, name, r->error_code);
    regs_dump(r);
    panic("unhandled exception in the kernel");
}

void interrupt_dispatch(struct regs *r)
{
    interrupt_handler h = r->vector < sizeof handlers / sizeof handlers[0]
                        ? handlers[r->vector] : NULL;

    if (r->vector < IRQ_BASE) {                 /* an exception */
        if (h)
            h(r);
        else
            unhandled_exception(r);
        return;
    }

    int irq = r->vector - IRQ_BASE;
    if (pic_is_spurious(irq))
        return;                                 /* no EOI for those */
    if (h)
        h(r);
    pic_eoi(irq);
}
