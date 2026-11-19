/* boot_info.h — what stage 2 hands the kernel. Must match boot/layout.inc. */
#ifndef SPROUT_BOOT_INFO_H
#define SPROUT_BOOT_INFO_H

#include <stdint.h>

#define BOOT_MAGIC 0x54525053u          /* "SPRT", in EAX at kernel entry */

enum e820_type {
    E820_USABLE = 1,
    E820_RESERVED = 2,
    E820_ACPI_RECLAIMABLE = 3,
    E820_ACPI_NVS = 4,
    E820_BAD = 5,
};

/* One entry of the BIOS memory map, exactly as int 0x15 E820 wrote it. */
struct e820_entry {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} __attribute__((packed));

struct boot_info {
    uint32_t boot_drive;
    uint32_t e820_count;
    uint32_t e820_map;      /* physical address of e820_count entries */
} __attribute__((packed));

#endif
