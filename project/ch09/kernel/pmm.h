/* pmm.h — the physical memory manager: hands out 4 KiB frames of RAM. */
#ifndef SPROUT_PMM_H
#define SPROUT_PMM_H

#include <stdint.h>

#define FRAME_SIZE      4096u
#define PMM_FIRST       0x00200000u     /* below: the kernel's 1 MiB and the
                                           BIOS's first MiB, never handed out */
#define PMM_LIMIT       0x40000000u     /* sprout manages the first 1 GiB */

/* Build the bitmap from the memory map (memmap_init must come first). */
void pmm_init(void);

/* The physical address of a free frame, now in use; 0 if none is left.
 * The frame's contents are whatever was there before. */
uint32_t pmm_alloc(void);

/* Return a frame. Panics on an address that was never handed out. */
void pmm_free(uint32_t frame);

uint32_t pmm_free_frames(void);
uint32_t pmm_total_frames(void);

/* "frames: N of M free ..." */
void pmm_print(void);

/* One past the highest managed byte of RAM. */
uint32_t pmm_top(void);

#endif
