/* memmap.c — keep, print and sum the E820 memory map. */
#include "memmap.h"

#include "kprintf.h"

static struct e820_entry map[MEMMAP_MAX];
static uint32_t count;
static uint32_t boot_drive;

void memmap_init(const struct boot_info *bi)
{
    const struct e820_entry *src =
        (const struct e820_entry *)(uintptr_t)bi->e820_map;

    count = bi->e820_count <= MEMMAP_MAX ? bi->e820_count : MEMMAP_MAX;
    for (uint32_t i = 0; i < count; i++)
        map[i] = src[i];
    boot_drive = bi->boot_drive;
}

uint32_t memmap_count(void)
{
    return count;
}

const struct e820_entry *memmap_entry(uint32_t i)
{
    return &map[i];
}

uint32_t memmap_boot_drive(void)
{
    return boot_drive;
}

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

uint64_t memmap_usable(void)
{
    uint64_t usable = 0;
    for (uint32_t i = 0; i < count; i++)
        if (map[i].type == E820_USABLE)
            usable += map[i].length;
    return usable;
}

void memmap_print(void)
{
    kprintf("memory map (%u entries from E820):\n", (unsigned)count);
    for (uint32_t i = 0; i < count; i++) {
        uint64_t last = map[i].base + map[i].length - 1;
        kprintf("  %016llx-%016llx  %s\n", map[i].base, last,
                e820_type_name(map[i].type));
    }
    uint64_t usable = memmap_usable();
    kprintf("usable RAM: %llu KiB (%llu MiB)\n", usable >> 10, usable >> 20);
}
