/* paging.h — the page directory, page tables and page faults. */
#ifndef SPROUT_PAGING_H
#define SPROUT_PAGING_H

#include <stdbool.h>
#include <stdint.h>

#define PAGE_SIZE       4096u

/* Bits in page directory and page table entries (Intel SDM Vol. 3A 4.3). */
#define PAGE_PRESENT    0x001u
#define PAGE_WRITE      0x002u      /* clear: read-only */
#define PAGE_USER       0x004u      /* clear: kernel only (Chapter 9) */
#define PAGE_ACCESSED   0x020u      /* set by the CPU on any access */
#define PAGE_DIRTY      0x040u      /* set by the CPU on a write */
#define PAGE_FRAME      0xFFFFF000u /* the physical address bits */

/* Identity-map RAM, unmap page 0, make the kernel's code read-only,
 * install the page fault handler and turn paging on. */
void paging_init(void);

/* Map the page at virt to the frame at phys (both 4 KiB-aligned) with
 * flags (PAGE_WRITE, PAGE_USER); PAGE_PRESENT is implied. */
void paging_map(uint32_t virt, uint32_t phys, uint32_t flags);

void paging_unmap(uint32_t virt);

/* The page table entry for virt, or 0 if there is none. */
uint32_t paging_lookup(uint32_t virt);

/* Print the mapped ranges, like QEMU's "info mem". */
void paging_print(void);

#endif
