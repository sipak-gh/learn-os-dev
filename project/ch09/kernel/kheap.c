/* kheap.c — see kheap.h.
 *
 * lib/heap.c does the bookkeeping; it only needs a way to get more
 * memory right after the end of the heap. That is where paging pays off:
 * the heap is one contiguous range of *virtual* addresses at 0xD0000000,
 * and each new page of it is backed by whichever physical frame
 * pmm_alloc() returns, wherever it is.
 */
#include "kheap.h"

#include <stdint.h>

#include "heap.h"
#include "io.h"
#include "kprintf.h"
#include "paging.h"
#include "pmm.h"

static struct heap heap;
static uint32_t brk = KHEAP_START;      /* end of the heap */
static uint32_t mapped = KHEAP_START;   /* end of the mapped pages */

/* lib/heap.c's more(): extend the heap by n bytes, mapping pages as needed. */
static void *kheap_more(size_t n)
{
    if (n > KHEAP_START + KHEAP_MAX - brk)
        return NULL;
    while (mapped < brk + n) {
        uint32_t frame = pmm_alloc();
        if (frame == 0)
            return NULL;
        paging_map(mapped, frame, PAGE_WRITE);
        mapped += PAGE_SIZE;
    }
    uint32_t old = brk;
    brk += n;
    return (void *)old;
}

void kheap_init(void)
{
    if (!heap_init(&heap, kheap_more))
        panic("kheap: no memory for the heap");
}

/* Tasks share the heap, and a task switch in the middle of heap_alloc
 * would let another task see half-updated headers: every entry point
 * runs with interrupts disabled (Chapter 8). */
void *kmalloc(size_t n)
{
    uint32_t flags = irq_save();
    void *p = heap_alloc(&heap, n);
    irq_restore(flags);
    return p;
}

bool kheap_free(void *p)
{
    uint32_t flags = irq_save();
    bool ok = heap_free(&heap, p);
    irq_restore(flags);
    return ok;
}

void kfree(void *p)
{
    if (p != NULL && !kheap_free(p))
        panic("kfree: %p is not a block from kmalloc (freed twice?)", p);
}

struct totals {
    unsigned blocks, used, free;
};

static void print_block(void *payload, size_t size, bool used, void *arg)
{
    struct totals *t = arg;

    kprintf("  %p  %6u bytes  %s\n", payload, (unsigned)size,
            used ? "in use" : "free");
    t->blocks++;
    if (used)
        t->used += size;
    else
        t->free += size;
}

void kheap_print(void)
{
    struct totals t = { 0, 0, 0 };
    uint32_t flags = irq_save();

    kprintf("heap at %p, %u KiB mapped\n", (void *)KHEAP_START,
            (unsigned)((mapped - KHEAP_START) >> 10));
    heap_walk(&heap, print_block, &t);
    kprintf("%u block%s: %u bytes in use, %u free\n",
            t.blocks, t.blocks == 1 ? "" : "s", t.used, t.free);
    irq_restore(flags);
}
