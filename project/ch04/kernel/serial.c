/* serial.c — a driver for the 16550 UART at COM1, without interrupts.
 *
 * The same eight registers the boot sector programmed, now from C. Offsets
 * from the base port: 0 data (or divisor low byte while DLAB=1), 1
 * interrupt enable (or divisor high), 2 FIFO control, 3 line control, 4
 * modem control, 5 line status.
 */
#include "serial.h"

#include "io.h"

#define COM1        0x3F8
#define REG_DATA    0
#define REG_IER     1
#define REG_FCR     2
#define REG_LCR     3
#define REG_MCR     4
#define REG_LSR     5

#define LCR_8N1     0x03
#define LCR_DLAB    0x80
#define LSR_THRE    0x20        /* transmit holding register empty */

void serial_init(void)
{
    outb(COM1 + REG_IER, 0x00);         /* no interrupts (yet) */
    outb(COM1 + REG_LCR, LCR_DLAB);
    outb(COM1 + REG_DATA, 3);           /* divisor 3: 38400 baud */
    outb(COM1 + REG_IER, 0);
    outb(COM1 + REG_LCR, LCR_8N1);
    outb(COM1 + REG_FCR, 0xC7);         /* enable + clear FIFOs, 14-byte threshold */
    outb(COM1 + REG_MCR, 0x03);         /* DTR + RTS: "we are here" */
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
