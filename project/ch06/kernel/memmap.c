/* memmap.c — print and sum the E820 memory map. */
#include "memmap.h"

#include "kprintf.h"

static const char *e820_type_name(uint32_t type)
{
    switch (type) {
    case E820_USABLE:           return "usable";
    case E820_RESERVED:         return "reserved";
    case E820_ACPI_RECLAIMABLE: return "ACPI reclaimable";
    case E820_ACPI_NVS:         return "ACPI NVS";
    case E820_BAD:              return "bad memory";
    default:                    return "unknown";
    }
}

static const struct e820_entry *entries(const struct boot_info *bi)
{
    return (const struct e820_entry *)(uintptr_t)bi->e820_map;
}

uint64_t memmap_usable(const struct boot_info *bi)
{
    uint64_t usable = 0;
    for (uint32_t i = 0; i < bi->e820_count; i++)
        if (entries(bi)[i].type == E820_USABLE)
            usable += entries(bi)[i].length;
    return usable;
}

void memmap_print(const struct boot_info *bi)
{
    const struct e820_entry *map = entries(bi);

    kprintf("memory map (%u entries from E820):\n", (unsigned)bi->e820_count);
    for (uint32_t i = 0; i < bi->e820_count; i++) {
        uint64_t last = map[i].base + map[i].length - 1;
        kprintf("  %016llx-%016llx  %s\n", map[i].base, last,
                e820_type_name(map[i].type));
    }
    uint64_t usable = memmap_usable(bi);
    kprintf("usable RAM: %llu KiB (%llu MiB)\n", usable >> 10, usable >> 20);
}
