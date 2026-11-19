/* interrupts.h — the IDT, the saved-register frame, and handler registration. */
#ifndef SPROUT_INTERRUPTS_H
#define SPROUT_INTERRUPTS_H

#include <stdint.h>

#define IRQ_BASE    32          /* the PICs deliver IRQ n as vector 32 + n */
#define IRQ_TIMER   0
#define IRQ_KEYBOARD 1
#define IRQ_COM1    4
#define VECTOR_SYSCALL 0x80     /* int 0x80, from user mode */

/* What isr.asm leaves on the stack, lowest address first: the order is
 * the reverse of the pushes. */
struct regs {
    uint32_t gs, fs, es, ds;                            /* isr_common */
    uint32_t edi, esi, ebp, esp_pusha, ebx, edx, ecx, eax; /* pusha */
    uint32_t vector, error_code;                        /* the stub */
    uint32_t eip, cs, eflags;                           /* the CPU */
    uint32_t user_esp, user_ss;     /* the CPU, only if we came from ring 3 */
};

/* Did the interrupt arrive while ring 3 code ran? (CS's low two bits are
 * the privilege level the CPU was running at.) */
#define FROM_USER(r) (((r)->cs & 3) == 3)

typedef void (*interrupt_handler)(struct regs *r);

/* Fill and load the IDT. Interrupts stay disabled. */
void idt_init(void);

/* Call handler for vector (0-31 exceptions, 32-47 IRQs, VECTOR_SYSCALL). */
void interrupt_register(uint8_t vector, interrupt_handler handler);

/* Called by isr_common for every interrupt. */
void interrupt_dispatch(struct regs *r);

/* Print every saved register (used by the default exception handler). */
void regs_dump(const struct regs *r);

#endif
