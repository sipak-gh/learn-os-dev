/* ksyms.c — see ksyms.h. */
#include "ksyms.h"

#include <stddef.h>

/* From build/symtab.asm (tools/mksyms.sh); ksyms_addr is sorted. */
extern const uint32_t ksyms_count;
extern const uint32_t ksyms_addr[];
extern const char *const ksyms_name[];

/* From linker.ld. */
extern const char __text_end[];

const char *ksyms_lookup(uint32_t addr, uint32_t *start)
{
    if (ksyms_count == 0 || addr < ksyms_addr[0]
        || addr >= (uint32_t)__text_end)
        return NULL;

    /* Binary search for the last function that starts at or below addr. */
    uint32_t lo = 0, hi = ksyms_count;     /* the answer is in [lo, hi) */
    while (hi - lo > 1) {
        uint32_t mid = lo + (hi - lo) / 2;
        if (ksyms_addr[mid] <= addr)
            lo = mid;
        else
            hi = mid;
    }
    *start = ksyms_addr[lo];
    return ksyms_name[lo];
}

uint32_t ksyms_size(void)
{
    return ksyms_count;
}
