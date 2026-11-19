/* heap.h — malloc and free for a region that can grow: first fit,
 * boundary tags, coalescing.
 *
 * The heap is one contiguous range of memory cut into blocks. Each block
 * starts with a 4-byte header and ends with a 4-byte footer; both hold the
 * block's size in bytes (a multiple of 8, header and footer included) with
 * bit 0 set while the block is in use:
 *
 *     | hdr | payload ...             | ftr | hdr | payload ...
 *           ^ the pointer heap_alloc returns (8-byte aligned)
 *
 * The footer is what lets heap_free look at the block *before* the one
 * being freed, so a freed block merges with free neighbours on both sides.
 * The layout is the "implicit free list" of Bryant and O'Hallaron's
 * Computer Systems: A Programmer's Perspective, section 9.9.
 */
#ifndef SPROUT_HEAP_H
#define SPROUT_HEAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HEAP_ALIGN  8
#define HEAP_CHUNK  4096            /* grow by at least this much */

struct heap {
    /* Like Unix sbrk: make n more bytes available right after the current
     * end of the heap and return their address, or NULL if there is no
     * more memory. The first call fixes the start; it must be 8-aligned. */
    void *(*more)(size_t n);
    uint8_t *first;                 /* payload of the first real block */
};

/* Set up an empty heap. Returns false if more() fails. */
bool heap_init(struct heap *h, void *(*more)(size_t n));

/* At least n bytes, 8-byte aligned; NULL if n is 0 or memory runs out. */
void *heap_alloc(struct heap *h, size_t n);

/* Give back a block from heap_alloc. Returns false, and changes nothing,
 * if p is not the start of a block in use (a double free, say). */
bool heap_free(struct heap *h, void *p);

/* Call fn for every block, in address order. size is the payload size
 * (block size minus header and footer). */
void heap_walk(const struct heap *h,
               void (*fn)(void *payload, size_t size, bool used, void *arg),
               void *arg);

/* Check every header against its footer and that no two free blocks are
 * neighbours (they should have been merged). */
bool heap_check(const struct heap *h);

#endif
