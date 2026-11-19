/* serial.h — the first serial port (COM1), polled. */
#ifndef SPROUT_SERIAL_H
#define SPROUT_SERIAL_H

void serial_init(void);
void serial_putc(char c);       /* "\n" goes out as "\r\n" */

#endif
