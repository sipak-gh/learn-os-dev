/* pic.c — the 8259A PIC pair.
 *
 * A PC has two 8259As: the master handles IRQ 0-7, the slave IRQ 8-15 and
 * is wired to the master's IRQ 2. The BIOS programs the master to deliver
 * IRQ 0-7 as vectors 8-15, which in protected mode are CPU exceptions
 * (8 is the double fault). So the first thing we do is reprogram both
 * with the four initialization command words, ICW1-ICW4.
 */
#include "pic.h"

#include "io.h"

#define PIC1_CMD    0x20
#define PIC1_DATA   0x21
#define PIC2_CMD    0xA0
#define PIC2_DATA   0xA1

#define ICW1_INIT   0x10    /* start initialization */
#define ICW1_ICW4   0x01    /* ... and ICW4 will follow */
#define ICW4_8086   0x01    /* 8086 mode (not 8080) */
#define CASCADE_IRQ 2       /* the slave hangs off the master's IRQ 2 */
#define OCW3_READ_ISR 0x0B  /* next read of the command port: the ISR */
#define PIC_EOI     0x20    /* non-specific end of interrupt */

/* A write to the unused port 0x80 takes about a microsecond: old PICs
 * need a moment between initialization words. */
static inline void io_wait(void)
{
    outb(0x80, 0);
}

void pic_init(uint8_t base)
{
    outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);  io_wait();
    outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);  io_wait();
    outb(PIC1_DATA, base);                  io_wait();  /* ICW2: vector base */
    outb(PIC2_DATA, base + 8);              io_wait();
    outb(PIC1_DATA, 1 << CASCADE_IRQ);      io_wait();  /* ICW3: slave on IRQ 2 */
    outb(PIC2_DATA, CASCADE_IRQ);           io_wait();  /* ICW3: my cascade id */
    outb(PIC1_DATA, ICW4_8086);             io_wait();
    outb(PIC2_DATA, ICW4_8086);             io_wait();

    /* Mask everything except the cascade line; drivers unmask their IRQ. */
    outb(PIC1_DATA, 0xFF & ~(1 << CASCADE_IRQ));
    outb(PIC2_DATA, 0xFF);
}

void pic_eoi(int irq)
{
    if (irq >= 8)
        outb(PIC2_CMD, PIC_EOI);
    outb(PIC1_CMD, PIC_EOI);
}

void pic_unmask(int irq)
{
    uint16_t port = irq < 8 ? PIC1_DATA : PIC2_DATA;
    outb(port, inb(port) & ~(1 << (irq & 7)));
}

void pic_mask(int irq)
{
    uint16_t port = irq < 8 ? PIC1_DATA : PIC2_DATA;
    outb(port, inb(port) | (1 << (irq & 7)));
}

/* A spurious IRQ arrives as the lowest-priority line (7 or 15) but its bit
 * is not set in the in-service register. It must not be acknowledged,
 * except that a spurious IRQ 15 did go through the master's IRQ 2, which
 * still needs its EOI. */
bool pic_is_spurious(int irq)
{
    if (irq != 7 && irq != 15)
        return false;

    uint16_t cmd = irq == 7 ? PIC1_CMD : PIC2_CMD;
    outb(cmd, OCW3_READ_ISR);
    if (inb(cmd) & 0x80)
        return false;

    if (irq == 15)
        outb(PIC1_CMD, PIC_EOI);
    return true;
}
