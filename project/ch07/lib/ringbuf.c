/* ringbuf.c — see ringbuf.h. */
#include "ringbuf.h"

void ringbuf_init(struct ringbuf *rb)
{
    rb->head = 0;
    rb->tail = 0;
}

size_t ringbuf_count(const struct ringbuf *rb)
{
    return rb->head - rb->tail;
}

bool ringbuf_put(struct ringbuf *rb, char c)
{
    if (ringbuf_count(rb) == RINGBUF_SIZE)
        return false;
    rb->data[rb->head % RINGBUF_SIZE] = c;
    rb->head++;                         /* publish only after the write */
    return true;
}

bool ringbuf_get(struct ringbuf *rb, char *c)
{
    if (ringbuf_count(rb) == 0)
        return false;
    *c = rb->data[rb->tail % RINGBUF_SIZE];
    rb->tail++;
    return true;
}
