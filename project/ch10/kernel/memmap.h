/* memmap.h — the BIOS memory map, copied out of low memory.
 *
 * Stage 2 left the E820 entries at 0x600 and boot_info at 0x500, both in
 * the first page of memory. Paging (Chapter 7) unmaps that page to catch
 * NULL pointers, so memmap_init copies everything we need first.
 */
#ifndef SPROUT_MEMMAP_H
#define SPROUT_MEMMAP_H

#include <stdint.h>

#include "boot_info.h"

#define MEMMAP_MAX  32              /* E820_MAX in boot/layout.inc */

void memmap_init(const struct boot_info *bi);

uint32_t memmap_count(void);
const struct e820_entry *memmap_entry(uint32_t i);
uint32_t memmap_boot_drive(void);

void memmap_print(void);

/* Bytes of RAM marked usable. */
uint64_t memmap_usable(void);

#endif
