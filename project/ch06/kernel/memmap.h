/* memmap.h — the BIOS memory map that stage 2 left in boot_info. */
#ifndef SPROUT_MEMMAP_H
#define SPROUT_MEMMAP_H

#include <stdint.h>

#include "boot_info.h"

void memmap_print(const struct boot_info *bi);

/* Bytes of RAM marked usable. */
uint64_t memmap_usable(const struct boot_info *bi);

#endif
