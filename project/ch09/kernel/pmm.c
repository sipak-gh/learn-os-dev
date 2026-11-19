/* pmm.c — see pmm.h.
 *
 * One bit per 4 KiB frame of the first 1 GiB: 262144 bits, 32 KiB of
 * .bss. A set bit means "not available": in use, or not RAM at all. We
 * start with every bit set and clear only the frames that lie entirely
 * inside a usable E820 range, at or above PMM_FIRST.
 *
 * Why skip everything below 2 MiB? The first MiB holds the BIOS's data,
 * the VGA buffer and our loader; the second holds the kernel (and its
 * .bss, which is where this bitmap lives). Reserving both outright keeps
 * the numbers the same whatever size the kernel is today.
 */
#include "pmm.h"

#include <stdbool.h>

#include "bitmap.h"
#include "kprintf.h"
#include "memmap.h"
#include "string.h"

#define NFRAMES (PMM_LIMIT / FRAME_SIZE)

extern char __kernel_end[];             /* kernel/linker.ld */

static uint32_t bitmap[BITMAP_WORDS(NFRAMES)];
static uint32_t total, free_count, top;

void pmm_init(void)
{
    kassert((uint32_t)__kernel_end <= PMM_FIRST);

    memset(bitmap, 0xFF, sizeof bitmap);
    for (uint32_t i = 0; i < memmap_count(); i++) {
        const struct e820_entry *e = memmap_entry(i);
        if (e->type != E820_USABLE)
            continue;

        /* Whole frames only: round the start up and the end down. */
        uint64_t start = (e->base + FRAME_SIZE - 1) & ~(uint64_t)(FRAME_SIZE - 1);
        uint64_t end = (e->base + e->length) & ~(uint64_t)(FRAME_SIZE - 1);
        if (start < PMM_FIRST)
            start = PMM_FIRST;
        if (end > PMM_LIMIT)
            end = PMM_LIMIT;

        for (uint64_t a = start; a < end; a += FRAME_SIZE) {
            bitmap_clear(bitmap, (uint32_t)(a / FRAME_SIZE));
            total++;
        }
        if (end > top)
            top = (uint32_t)end;
    }
    free_count = total;
}

uint32_t pmm_alloc(void)
{
    uint32_t i = bitmap_find_clear(bitmap, NFRAMES, PMM_FIRST / FRAME_SIZE);
    if (i == NFRAMES)
        return 0;
    bitmap_set(bitmap, i);
    free_count--;
    return i * FRAME_SIZE;
}

void pmm_free(uint32_t frame)
{
    uint32_t i = frame / FRAME_SIZE;

    if (frame % FRAME_SIZE != 0 || frame < PMM_FIRST || frame >= top
            || !bitmap_test(bitmap, i))
        panic("pmm_free: %08x was not allocated", (unsigned)frame);
    bitmap_clear(bitmap, i);
    free_count++;
}

uint32_t pmm_free_frames(void)
{
    return free_count;
}

uint32_t pmm_total_frames(void)
{
    return total;
}

uint32_t pmm_top(void)
{
    return top;
}

void pmm_print(void)
{
    kprintf("frames: %u of %u free (%u KiB), 4 KiB each, %08x-%08x\n",
            (unsigned)free_count, (unsigned)total,
            (unsigned)(free_count * (FRAME_SIZE / 1024)),
            (unsigned)PMM_FIRST, (unsigned)top);
}
