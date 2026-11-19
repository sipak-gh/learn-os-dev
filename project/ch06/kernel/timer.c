/* timer.c — the 8253/8254 programmable interval timer, channel 0.
 *
 * The PIT counts down from a 16-bit divisor at 1.193182 MHz (the original
 * PC's 14.31818 MHz crystal divided by 12; also a third of the NTSC
 * colour-burst frequency) and raises IRQ 0 each time the count runs out.
 */
#include "timer.h"

#include "interrupts.h"
#include "io.h"
#include "kprintf.h"
#include "pic.h"

#define PIT_HZ          1193182u
#define PIT_CHANNEL0    0x40
#define PIT_COMMAND     0x43
/* Channel 0, write low byte then high byte, mode 3 (square wave), binary. */
#define PIT_CMD_CH0_SQUARE  0x36

static volatile uint32_t ticks;     /* changed by the interrupt handler */
static uint32_t hz;

static void timer_irq(struct regs *r)
{
    (void)r;
    ticks++;
}

void timer_init(uint32_t freq)
{
    uint32_t divisor = (PIT_HZ + freq / 2) / freq;   /* rounded */

    /* The divisor register has 16 bits: below 19 Hz it would not fit. */
    kassert(divisor >= 2 && divisor <= 0xFFFF);
    hz = freq;
    outb(PIT_COMMAND, PIT_CMD_CH0_SQUARE);
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, divisor >> 8);

    interrupt_register(IRQ_BASE + IRQ_TIMER, timer_irq);
    pic_unmask(IRQ_TIMER);
}

uint32_t timer_hz(void)
{
    return hz;
}

uint32_t timer_ticks(void)
{
    return ticks;
}

void timer_sleep(uint32_t n)
{
    uint32_t start = ticks;
    while (ticks - start < n)       /* correct even when ticks wraps */
        hlt();
}
