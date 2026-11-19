/* line.h — a line editor: turns typed characters into a line of text.
 *
 * Feed it one character at a time. Printable characters are stored and
 * echoed, backspace erases the last one (on screen too), Enter finishes
 * the line. Everything it shows goes through the echo callback, so the
 * same code runs in the kernel (echo = console_putc) and in unit tests.
 */
#ifndef SPROUT_LINE_H
#define SPROUT_LINE_H

#include <stdbool.h>
#include <stddef.h>

struct line {
    char *buf;
    size_t cap;                 /* bytes in buf, including the final NUL */
    size_t len;
    bool done;                  /* the previous character ended a line */
    void (*echo)(char c);
};

void line_init(struct line *l, char *buf, size_t cap, void (*echo)(char c));

/* Process c. Returns true when c was Enter: buf then holds the line,
 * NUL-terminated, until the next call. */
bool line_feed(struct line *l, char c);

#endif
