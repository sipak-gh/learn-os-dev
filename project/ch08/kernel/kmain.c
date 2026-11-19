/* kmain.c — sprout's C entry point: entry.asm calls kmain(magic, info). */
#include <stdint.h>

#include "boot_info.h"
#include "interrupts.h"
#include "io.h"
#include "keyboard.h"
#include "kheap.h"
#include "kprintf.h"
#include "memmap.h"
#include "paging.h"
#include "pic.h"
#include "pmm.h"
#include "serial.h"
#include "shell.h"
#include "task.h"
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
    memmap_init(bi);                /* copy it before paging hides page 0 */
    memmap_print();

    idt_init();                     /* so mistakes below are reported */
    pmm_init();
    paging_init();
    pmm_print();
    kprintf("paging: on; RAM identity-mapped up to %08x except page 0, "
            "kernel code read-only\n", (unsigned)pmm_top());
    kheap_init();
    kprintf("heap: kmalloc from %p\n", (void *)KHEAP_START);

    pic_init(IRQ_BASE);
    timer_init(100);
    keyboard_init();
    serial_enable_input();
    sti();
    kprintf("interrupts on: timer %u Hz, keyboard (IRQ 1), COM1 input (IRQ 4)\n",
            timer_hz());

    /* From here on, kmain is task 0, idle: it runs when nobody else can. */
    task_init();
    if (task_spawn("shell", shell_main) < 0)
        panic("cannot start the shell");
    kprintf("tasks: round robin, %u ms time slice; the shell is task 1\n",
            TASK_SLICE * 1000 / timer_hz());
    kprintf("type help for a list of commands\n");
    for (;;) {
        task_yield();
        hlt();                  /* until an interrupt; it may wake a task */
    }
}
