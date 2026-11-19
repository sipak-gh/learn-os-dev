/* elf.h — load a 32-bit ELF executable into a user address space. */
#ifndef SPROUT_ELF_H
#define SPROUT_ELF_H

#include <stdint.h>

/* Map and fill the program's PT_LOAD segments in space (a page directory
 * from paging_new_space). Returns NULL and sets *entry to the program's
 * first instruction, or returns what is wrong with the file. */
const char *elf_load(uint32_t *space, const uint8_t *file, uint32_t size,
                     uint32_t *entry);

#endif
