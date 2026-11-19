/* line.c — see line.h. */
#include "line.h"

#define BACKSPACE   '\b'
#define DELETE      0x7F            /* what many terminals send for backspace */

void line_init(struct line *l, char *buf, size_t cap, void (*echo)(char c))
{
    l->buf = buf;
    l->cap = cap;
    l->len = 0;
    l->done = false;
    l->echo = echo;
    buf[0] = '\0';
}

bool line_feed(struct line *l, char c)
{
    if (l->done) {                  /* start a new line */
        l->len = 0;
        l->done = false;
    }

    if (c == '\n') {
        l->buf[l->len] = '\0';
        l->done = true;
        l->echo('\n');
        return true;
    }

    if (c == BACKSPACE || c == DELETE) {
        if (l->len > 0) {
            l->len--;
            l->echo('\b');          /* back, blank it out, back again */
            l->echo(' ');
            l->echo('\b');
        }
        return false;
    }

    /* Printable ASCII only, and keep room for the NUL. */
    if (c >= ' ' && c <= '~' && l->len + 1 < l->cap) {
        l->buf[l->len++] = c;
        l->echo(c);
    }
    return false;
}
