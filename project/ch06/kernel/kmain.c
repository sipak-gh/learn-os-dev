/* kmain.c — sprout's C entry point: entry.asm calls kmain(magic, info). */
#include <stdint.h>

#include "boot_info.h"
#include "interrupts.h"
#include "io.h"
#include "keyboard.h"
#include "kprintf.h"
#include "memmap.h"
#include "pic.h"
#include "serial.h"
#include "shell.h"
#include "timer.h"
#include "version.h"
#include "vga.h"

/* Defined by kernel/linker.ld: only their addresses mean anything. */
extern char __kernel_start[];

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
    memmap_print(bi);

    idt_init();
    pic_init(IRQ_BASE);
    timer_init(100);
    keyboard_init();
    serial_enable_input();
    sti();
    kprintf("interrupts on: timer %u Hz, keyboard (IRQ 1), COM1 input (IRQ 4)\n",
            timer_hz());
    kprintf("type help for a list of commands\n");

    shell_run(bi);
}
