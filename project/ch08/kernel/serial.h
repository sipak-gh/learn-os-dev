/* serial.h — the first serial port (COM1): polled output, interrupt input. */
#ifndef SPROUT_SERIAL_H
#define SPROUT_SERIAL_H

void serial_init(void);
void serial_putc(char c);       /* "\n" goes out as "\r\n" */

/* Deliver received characters to input_put() from IRQ 4. */
void serial_enable_input(void);

#endif
