/* bitmap.h — an array of bits, packed 32 to a word.
 *
 * Bit i lives in word i / 32, at bit position i % 32. The physical memory
 * manager keeps one bit per 4 KiB frame: set = in use.
 */
#ifndef SPROUT_BITMAP_H
#define SPROUT_BITMAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Words needed for nbits bits. */
#define BITMAP_WORDS(nbits) (((nbits) + 31) / 32)

void bitmap_set(uint32_t *map, size_t i);
void bitmap_clear(uint32_t *map, size_t i);
bool bitmap_test(const uint32_t *map, size_t i);

/* The first clear bit at index start or later, or nbits if there is none. */
size_t bitmap_find_clear(const uint32_t *map, size_t nbits, size_t start);

#endif
