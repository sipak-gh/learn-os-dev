/* kprintf.c — kprintf() and panic() on top of lib/fmt.c. */
#include "kprintf.h"

#include <stdarg.h>

#include "backtrace.h"
#include "fmt.h"
#include "io.h"
#include "serial.h"
#include "vga.h"

void console_init(void)
{
    serial_init();
    vga_init();
}

/* The console is shared by all tasks: a task switch in the middle of
 * vga_putc would leave the cursor half-updated, and one in the middle of a
 * kprintf would mix two lines. So both run with interrupts disabled. */
void console_putc(char c)
{
    uint32_t flags = irq_save();
    vga_putc(c);
    serial_putc(c);
    irq_restore(flags);
}

static void console_out(char c, void *ctx)
{
    (void)ctx;
    console_putc(c);
}

int kprintf(const char *fmt, ...)
{
    va_list ap;
    uint32_t flags = irq_save();
    va_start(ap, fmt);
    int n = fmt_format(console_out, NULL, fmt, ap);
    va_end(ap);
    irq_restore(flags);
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
    kprintf("\n");
    backtrace();
    kprintf("system halted\n");
    for (;;)
        hlt();
}
