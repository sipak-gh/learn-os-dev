/* kprintf.h — kernel console output and panic(). */
#ifndef SPROUT_KPRINTF_H
#define SPROUT_KPRINTF_H

/* Every character goes to the VGA screen and to COM1. */
void console_init(void);
void console_putc(char c);

int kprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/* Print "PANIC" with the message and where it came from, stop forever. */
#define panic(...) panic_at(__FILE__, __LINE__, __VA_ARGS__)
void panic_at(const char *file, int line, const char *fmt, ...)
    __attribute__((format(printf, 3, 4), noreturn));

/* assert() for the kernel: panics with the failed expression. */
#define kassert(cond)                                               \
    do {                                                            \
        if (!(cond))                                                \
            panic("assertion failed: %s", #cond);                   \
    } while (0)

#endif
