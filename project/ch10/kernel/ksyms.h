/* ksyms.h — the kernel's own symbol table (Chapter 10).
 *
 * tools/mksyms.sh generates the table from kernel.elf, and the Makefile
 * links it into the kernel, so a panic can print function names.
 */
#ifndef SPROUT_KSYMS_H
#define SPROUT_KSYMS_H

#include <stdint.h>

/* The name of the kernel function that contains addr, and that function's
 * first address in *start. NULL if addr is not in the kernel's code. */
const char *ksyms_lookup(uint32_t addr, uint32_t *start);

/* How many functions the table has (0 in the first link). */
uint32_t ksyms_size(void);

#endif
