/* heap.c — see heap.h.
 *
 * Blocks are named by their payload address bp. The heap starts with
 * 4 bytes of padding (so payloads land on 8-byte boundaries), a "prologue"
 * block of size 8 that is always in use (header and footer, no payload),
 * and ends with an "epilogue": a lone header of size 0, in use. With
 * those two in place, every real block has a neighbour on each side, and
 * merging never needs to check for the edges of the heap.
 *
 *   | pad | 8/1 | 8/1 | hdr | payload ... | ftr | ... | 0/1 |
 *               ^ prologue  ^ h->first                  ^ epilogue
 */
#include "heap.h"

#define USED        1u
#define OVERHEAD    8               /* header + footer */
#define MIN_BLOCK   16              /* header + 8 bytes of payload + footer */

static uint32_t get(const uint8_t *p) { return *(const uint32_t *)p; }
static void put(uint8_t *p, uint32_t v) { *(uint32_t *)p = v; }

static uint8_t *header(uint8_t *bp) { return bp - 4; }
static uint32_t block_size(uint8_t *bp) { return get(header(bp)) & ~7u; }
static bool block_used(uint8_t *bp) { return get(header(bp)) & USED; }
static uint8_t *footer(uint8_t *bp) { return bp + block_size(bp) - OVERHEAD; }
static uint8_t *next_block(uint8_t *bp) { return bp + block_size(bp); }
static uint8_t *prev_block(uint8_t *bp)
{
    return bp - (get(bp - OVERHEAD) & ~7u);    /* the previous footer */
}

static void set_block(uint8_t *bp, uint32_t size, uint32_t used)
{
    put(header(bp), size | used);
    put(bp + size - OVERHEAD, size | used);
}

/* Merge the free block bp with free neighbours; return the merged block. */
static uint8_t *coalesce(uint8_t *bp)
{
    uint32_t size = block_size(bp);
    bool prev_used = block_used(prev_block(bp));
    bool next_used = block_used(next_block(bp));

    if (!next_used)
        size += block_size(next_block(bp));
    if (!prev_used) {
        bp = prev_block(bp);
        size += block_size(bp);
    }
    set_block(bp, size, 0);
    return bp;
}

/* Grow the heap by size bytes (a multiple of 8) and return the new free
 * block, merged with a free block before it. */
static uint8_t *extend(struct heap *h, uint32_t size)
{
    uint8_t *bp = h->more(size);
    if (bp == NULL)
        return NULL;
    /* bp is just past the old epilogue header, which becomes our header. */
    set_block(bp, size, 0);
    put(header(next_block(bp)), 0 | USED);      /* the new epilogue */
    return coalesce(bp);
}

bool heap_init(struct heap *h, void *(*more)(size_t n))
{
    h->more = more;
    uint8_t *p = more(4 * 4);
    if (p == NULL)
        return false;
    put(p, 0);                                  /* padding */
    put(p + 4, OVERHEAD | USED);                /* prologue header */
    put(p + 8, OVERHEAD | USED);                /* prologue footer */
    put(p + 12, 0 | USED);                      /* epilogue */
    h->first = p + 16;
    return true;
}

/* Use asize bytes of the free block bp; split off the rest if it is big
 * enough to be a block of its own. */
static void place(uint8_t *bp, uint32_t asize)
{
    uint32_t size = block_size(bp);

    if (size - asize >= MIN_BLOCK) {
        set_block(bp, asize, USED);
        set_block(next_block(bp), size - asize, 0);
    } else {
        set_block(bp, size, USED);
    }
}

void *heap_alloc(struct heap *h, size_t n)
{
    if (n == 0 || n > 0x7FFFFFF0u - OVERHEAD)
        return NULL;
    uint32_t asize = (uint32_t)(n + OVERHEAD + HEAP_ALIGN - 1) & ~(HEAP_ALIGN - 1u);
    if (asize < MIN_BLOCK)
        asize = MIN_BLOCK;

    /* First fit: the first free block that is big enough. */
    uint8_t *bp;
    for (bp = h->first; block_size(bp) > 0; bp = next_block(bp)) {
        if (!block_used(bp) && block_size(bp) >= asize) {
            place(bp, asize);
            return bp;
        }
    }

    bp = extend(h, asize > HEAP_CHUNK ? asize : HEAP_CHUNK);
    if (bp == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

/* Is p the payload of a block in use? Walks the heap: slow, but it
 * catches double frees and stray pointers instead of corrupting memory. */
static bool is_used_block(const struct heap *h, uint8_t *p)
{
    for (uint8_t *bp = h->first; block_size(bp) > 0; bp = next_block(bp))
        if (bp == p)
            return block_used(bp);
    return false;
}

bool heap_free(struct heap *h, void *p)
{
    uint8_t *bp = p;

    if (bp == NULL || !is_used_block(h, bp))
        return false;
    set_block(bp, block_size(bp), 0);
    coalesce(bp);
    return true;
}

void heap_walk(const struct heap *h,
               void (*fn)(void *payload, size_t size, bool used, void *arg),
               void *arg)
{
    for (uint8_t *bp = h->first; block_size(bp) > 0; bp = next_block(bp))
        fn(bp, block_size(bp) - OVERHEAD, block_used(bp), arg);
}

bool heap_check(const struct heap *h)
{
    bool prev_free = false;

    for (uint8_t *bp = h->first; block_size(bp) > 0; bp = next_block(bp)) {
        if (get(header(bp)) != get(footer(bp)))
            return false;
        if ((uintptr_t)bp % HEAP_ALIGN != 0 || block_size(bp) < MIN_BLOCK)
            return false;
        if (prev_free && !block_used(bp))
            return false;
        prev_free = !block_used(bp);
    }
    return true;
}
