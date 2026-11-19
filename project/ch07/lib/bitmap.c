/* bitmap.c — see bitmap.h. */
#include "bitmap.h"

void bitmap_set(uint32_t *map, size_t i)
{
    map[i / 32] |= 1u << (i % 32);
}

void bitmap_clear(uint32_t *map, size_t i)
{
    map[i / 32] &= ~(1u << (i % 32));
}

bool bitmap_test(const uint32_t *map, size_t i)
{
    return map[i / 32] & (1u << (i % 32));
}

size_t bitmap_find_clear(const uint32_t *map, size_t nbits, size_t start)
{
    size_t i = start;

    while (i < nbits) {
        /* A full word has no clear bit: skip all 32 at once. */
        if (i % 32 == 0 && map[i / 32] == 0xFFFFFFFFu) {
            i += 32;
            continue;
        }
        if (!bitmap_test(map, i))
            return i;
        i++;
    }
    return nbits;
}
