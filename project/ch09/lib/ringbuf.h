/* ringbuf.h — a fixed-size byte queue for one producer and one consumer.
 *
 * The keyboard and serial interrupt handlers put characters in; the shell
 * takes them out. head and tail count forever (they wrap around at 2^32,
 * which unsigned arithmetic handles); only their difference and their
 * values modulo RINGBUF_SIZE matter.
 */
#ifndef SPROUT_RINGBUF_H
#define SPROUT_RINGBUF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RINGBUF_SIZE 256                /* a power of two */

struct ringbuf {
    volatile uint32_t head;             /* next slot to write */
    volatile uint32_t tail;             /* next slot to read */
    char data[RINGBUF_SIZE];
};

void ringbuf_init(struct ringbuf *rb);
size_t ringbuf_count(const struct ringbuf *rb);

/* Add c; returns false (and drops c) when the buffer is full. */
bool ringbuf_put(struct ringbuf *rb, char c);

/* Remove the oldest character into *c; returns false when empty. */
bool ringbuf_get(struct ringbuf *rb, char *c);

#endif
