/* paging.h — the page directory, page tables and page faults. */
#ifndef SPROUT_PAGING_H
#define SPROUT_PAGING_H

#include <stdbool.h>
#include <stdint.h>

#define PAGE_SIZE       4096u

/* Bits in page directory and page table entries (Intel SDM Vol. 3A 4.3). */
#define PAGE_PRESENT    0x001u
#define PAGE_WRITE      0x002u      /* clear: read-only */
#define PAGE_USER       0x004u      /* set: ring 3 may use the page */
#define PAGE_ACCESSED   0x020u      /* set by the CPU on any access */
#define PAGE_DIRTY      0x040u      /* set by the CPU on a write */
#define PAGE_FRAME      0xFFFFF000u /* the physical address bits */

/* User programs live between these two addresses (Chapter 9); the
 * kernel never maps anything there. */
#define USER_BASE       0x40000000u
#define USER_TOP        0x80000000u

/* Identity-map RAM, unmap page 0, make the kernel's code read-only,
 * install the page fault handler and turn paging on. */
void paging_init(void);

/* Map the page at virt to the frame at phys (both 4 KiB-aligned) with
 * flags (PAGE_WRITE, PAGE_USER); PAGE_PRESENT is implied. These change
 * the kernel's part of the address space, which every task shares. */
void paging_map(uint32_t virt, uint32_t phys, uint32_t flags);

void paging_unmap(uint32_t virt);

/* The page table entry for virt, or 0 if there is none. */
uint32_t paging_lookup(uint32_t virt);

/* An address space is a page directory. A new one maps the kernel
 * exactly like the kernel's own (the same page tables), and nothing
 * between USER_BASE and USER_TOP. Returns NULL if out of memory. */
uint32_t *paging_new_space(void);

/* Free a space's user pages, its page tables and the directory itself. */
void paging_free_space(uint32_t *dir);

/* paging_map and paging_lookup for the user part of dir. */
bool paging_map_user(uint32_t *dir, uint32_t virt, uint32_t phys,
                     uint32_t flags);
uint32_t paging_lookup_in(const uint32_t *dir, uint32_t virt);

/* Make dir the current address space (load CR3); NULL means the kernel's. */
void paging_switch(uint32_t *dir);

/* Print the mapped ranges of dir (NULL: the kernel's), like QEMU's
 * "info mem". */
void paging_print(const uint32_t *dir);

#endif
