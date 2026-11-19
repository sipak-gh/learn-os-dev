/* divide.c — 64-bit division in 32-bit code needs a helper from libgcc. */
#include <stdint.h>

uint64_t average(uint64_t total, uint64_t count)
{
    return total / count;
}
