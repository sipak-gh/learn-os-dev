/* kmain.c — sprout's C entry point: entry.asm calls kmain(magic, info). */
#include <stddef.h>
#include <stdint.h>

#include "boot_info.h"
#include "interrupts.h"
#include "io.h"
#include "kprintf.h"
#include "pic.h"
#include "timer.h"
#include "version.h"
#include "vga.h"

/* Defined by kernel/linker.ld: only their addresses mean anything. */
extern char __kernel_start[];

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

static void print_memory_map(const struct boot_info *bi)
{
    const struct e820_entry *map =
        (const struct e820_entry *)(uintptr_t)bi->e820_map;
    uint64_t usable = 0;

    kprintf("memory map (%u entries from E820):\n", (unsigned)bi->e820_count);
    for (uint32_t i = 0; i < bi->e820_count; i++) {
        uint64_t last = map[i].base + map[i].length - 1;
        kprintf("  %016llx-%016llx  %s\n", map[i].base, last,
                e820_type_name(map[i].type));
        if (map[i].type == E820_USABLE)
            usable += map[i].length;
    }
    kprintf("usable RAM: %llu KiB (%llu MiB)\n", usable >> 10, usable >> 20);
}

/* int3 is a trap: the saved eip already points past it, so returning
 * resumes at the next instruction. Its opcode is the single byte 0xCC. */
static void breakpoint(struct regs *r)
{
    const uint8_t *after = (const uint8_t *)r->eip;
    kprintf("breakpoint: vector %u from cs=%04x, byte before eip: %02x\n",
            r->vector, r->cs, after[-1]);
}

void kmain(uint32_t magic, const struct boot_info *bi)
{
    console_init();
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    kprintf("sprout %s\n", SPROUT_VERSION);
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);

    if (magic != BOOT_MAGIC)
        panic("bad boot magic %x (expected %x)", (unsigned)magic, BOOT_MAGIC);

    /* Not the size: it changes with every edit, and tests/boot.expected
     * should not. */
    kprintf("kernel: running at %p, booted from drive %x\n",
            (void *)__kernel_start, (unsigned)bi->boot_drive);
    print_memory_map(bi);

    idt_init();
    kprintf("idt: vectors 0-31 (exceptions) and 32-47 (IRQs) installed\n");
    pic_init(IRQ_BASE);
    kprintf("pic: IRQ 0-15 remapped to vectors %u-%u\n",
            IRQ_BASE, IRQ_BASE + 15);

    interrupt_register(3, breakpoint);
    __asm__ volatile ("int3");
    kprintf("back in kmain after int3\n");

    timer_init(100);
    sti();
    kprintf("timer: PIT channel 0 at %u Hz, interrupts enabled\n", timer_hz());
    for (uint32_t i = 1; i <= 3; i++) {
        timer_sleep(timer_hz() / 2);
        kprintf("  %u.%u s\n", i / 2, i % 2 * 5);
    }

    kprintf("idle: halting until the next interrupt, forever\n");
    for (;;)
        hlt();
}
