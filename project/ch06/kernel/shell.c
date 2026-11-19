/* shell.c — read a line, split it into words, run the command. */
#include "shell.h"

#include <stddef.h>
#include <stdint.h>

#include "args.h"
#include "input.h"
#include "kprintf.h"
#include "line.h"
#include "memmap.h"
#include "power.h"
#include "string.h"
#include "timer.h"
#include "version.h"
#include "vga.h"

#define LINE_MAX    128
#define ARGS_MAX    8

struct command {
    const char *name;
    const char *usage;              /* arguments, for help */
    const char *help;
    void (*run)(int argc, char *argv[]);
};

static const struct boot_info *boot_info;
static void cmd_help(int argc, char *argv[]);    /* needs the table below */

static void cmd_echo(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++)
        kprintf("%s%s", argv[i], i + 1 < argc ? " " : "");
    kprintf("\n");
}

static void cmd_version(int argc, char *argv[])
{
    (void)argc, (void)argv;
    kprintf("sprout %s\n", SPROUT_VERSION);
}

static void cmd_mem(int argc, char *argv[])
{
    (void)argc, (void)argv;
    memmap_print(boot_info);
}

static void cmd_uptime(int argc, char *argv[])
{
    (void)argc, (void)argv;
    uint32_t t = timer_ticks(), hz = timer_hz();
    kprintf("up %u.%02u s (%u ticks at %u Hz)\n",
            t / hz, t % hz * 100 / hz, t, hz);
}

static void cmd_sleep(int argc, char *argv[])
{
    uint32_t ms;
    if (argc != 2 || !args_parse_u32(argv[1], &ms) || ms > 60000) {
        kprintf("usage: sleep MS (at most 60000)\n");
        return;
    }
    timer_sleep((ms * timer_hz() + 999) / 1000);    /* round up */
}

static void cmd_clear(int argc, char *argv[])
{
    (void)argc, (void)argv;
    vga_init();                     /* the screen only; COM1 has no "clear" */
}

static void cmd_crash(int argc, char *argv[])
{
    (void)argc, (void)argv;
    volatile int zero = 0;
    kprintf("%d\n", 1 / zero);
}

static void cmd_reboot(int argc, char *argv[])
{
    (void)argc, (void)argv;
    kprintf("rebooting\n");
    reboot();
}

static void cmd_poweroff(int argc, char *argv[])
{
    (void)argc, (void)argv;
    kprintf("powering off\n");
    poweroff();
}

static const struct command commands[] = {
    { "help",     "",       "list the commands",            cmd_help },
    { "echo",     "WORDS",  "print the words",              cmd_echo },
    { "version",  "",       "print the kernel version",     cmd_version },
    { "mem",      "",       "print the memory map",         cmd_mem },
    { "uptime",   "",       "time since the timer started", cmd_uptime },
    { "sleep",    "MS",     "wait MS milliseconds",         cmd_sleep },
    { "clear",    "",       "clear the screen",             cmd_clear },
    { "crash",    "",       "divide by zero",               cmd_crash },
    { "reboot",   "",       "restart the machine",          cmd_reboot },
    { "poweroff", "",       "turn the machine off (QEMU)",  cmd_poweroff },
    { NULL, NULL, NULL, NULL },
};

static void cmd_help(int argc, char *argv[])
{
    (void)argc, (void)argv;
    for (const struct command *c = commands; c->name; c++)
        kprintf("  %-8s %-6s %s\n", c->name, c->usage, c->help);
}

static void run(char *line)
{
    char *argv[ARGS_MAX];
    int argc = args_split(line, argv, ARGS_MAX);

    if (argc < 0) {
        kprintf("too many words (at most %d)\n", ARGS_MAX);
        return;
    }
    if (argc == 0)
        return;
    for (const struct command *c = commands; c->name; c++) {
        if (strcmp(argv[0], c->name) == 0) {
            c->run(argc, argv);
            return;
        }
    }
    kprintf("%s: unknown command (try help)\n", argv[0]);
}

void shell_run(const struct boot_info *bi)
{
    static char buf[LINE_MAX];
    struct line line;

    boot_info = bi;
    line_init(&line, buf, sizeof buf, console_putc);
    for (;;) {
        vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        kprintf(SHELL_PROMPT);
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        while (!line_feed(&line, input_getc()))
            ;
        run(buf);
    }
}
