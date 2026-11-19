/* serial.c — a driver for the 16550 UART at COM1.
 *
 * The same eight registers the boot sector programmed, now from C. Offsets
 * from the base port: 0 data (or divisor low byte while DLAB=1), 1
 * interrupt enable (or divisor high), 2 FIFO control, 3 line control, 4
 * modem control, 5 line status.
 */
#include "serial.h"

#include "input.h"
#include "interrupts.h"
#include "io.h"
#include "pic.h"

#define COM1        0x3F8
#define REG_DATA    0
#define REG_IER     1
#define REG_FCR     2
#define REG_LCR     3
#define REG_MCR     4
#define REG_LSR     5

#define LCR_8N1     0x03
#define LCR_DLAB    0x80
#define LSR_DR      0x01        /* data ready: a received byte is waiting */
#define LSR_THRE    0x20        /* transmit holding register empty */
#define IER_RX      0x01        /* interrupt when data is received */
#define MCR_DTR_RTS 0x03
#define MCR_OUT2    0x08        /* on a PC, connects the UART's IRQ line */

void serial_init(void)
{
    outb(COM1 + REG_IER, 0x00);         /* no interrupts (yet) */
    outb(COM1 + REG_LCR, LCR_DLAB);
    outb(COM1 + REG_DATA, 3);           /* divisor 3: 38400 baud */
    outb(COM1 + REG_IER, 0);
    outb(COM1 + REG_LCR, LCR_8N1);
    outb(COM1 + REG_FCR, 0x07);         /* enable + clear FIFOs, interrupt
                                           after every received byte */
    outb(COM1 + REG_MCR, MCR_DTR_RTS);  /* "we are here" */
}

static void serial_write(char c)
{
    while ((inb(COM1 + REG_LSR) & LSR_THRE) == 0)
        ;                               /* wait for room */
    outb(COM1 + REG_DATA, (unsigned char)c);
}

void serial_putc(char c)
{
    if (c == '\n')
        serial_write('\r');
    serial_write(c);
}

/* Terminals send CR for Enter and often DEL (0x7F) for Backspace; the
 * rest of the kernel wants '\n' and '\b'. */
static void serial_irq(struct regs *r)
{
    (void)r;
    while (inb(COM1 + REG_LSR) & LSR_DR) {
        char c = (char)inb(COM1 + REG_DATA);
        if (c == '\r')
            c = '\n';
        else if (c == 0x7F)
            c = '\b';
        input_put(c);
    }
}

void serial_enable_input(void)
{
    interrupt_register(IRQ_BASE + IRQ_COM1, serial_irq);
    outb(COM1 + REG_MCR, MCR_DTR_RTS | MCR_OUT2);
    outb(COM1 + REG_IER, IER_RX);
    pic_unmask(IRQ_COM1);
}
