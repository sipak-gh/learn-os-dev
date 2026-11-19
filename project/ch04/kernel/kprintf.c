/* kprintf.c — kprintf() and panic() on top of lib/fmt.c. */
#include "kprintf.h"

#include <stdarg.h>

#include "fmt.h"
#include "io.h"
#include "serial.h"
#include "vga.h"

void console_init(void)
{
    serial_init();
    vga_init();
}

void console_putc(char c)
{
    vga_putc(c);
    serial_putc(c);
}

static void console_out(char c, void *ctx)
{
    (void)ctx;
    console_putc(c);
}

int kprintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = fmt_format(console_out, NULL, fmt, ap);
    va_end(ap);
    return n;
}

void panic_at(const char *file, int line, const char *fmt, ...)
{
    cli();                              /* nothing may interrupt us now */
    vga_set_color(VGA_WHITE, VGA_RED);
    kprintf("\nPANIC at %s:%d: ", file, line);
    va_list ap;
    va_start(ap, fmt);
    fmt_format(console_out, NULL, fmt, ap);
    va_end(ap);
    kprintf("\nsystem halted\n");
    for (;;)
        hlt();
}
