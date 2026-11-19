/* set2.c — switch off the 8042's scancode translation.
 *
 * The controller's configuration byte (read with command 0x20, written
 * with 0x60) has bit 6 "translate set 2 to set 1". With it clear, port
 * 0x60 delivers what the keyboard really sends: scancode set 2.
 */
#include <stdint.h>

#include "io.h"

#define KBC_DATA        0x60
#define KBC_STATUS      0x64
#define KBC_COMMAND     0x64
#define STATUS_OUT_FULL 0x01
#define STATUS_IN_FULL  0x02
#define CMD_READ_CONFIG 0x20
#define CMD_WRITE_CONFIG 0x60
#define CONFIG_TRANSLATE 0x40

static void wait_writable(void)
{
    while (inb(KBC_STATUS) & STATUS_IN_FULL)
        ;
}

void kbd_translation_off(void)
{
    wait_writable();
    outb(KBC_COMMAND, CMD_READ_CONFIG);
    while (!(inb(KBC_STATUS) & STATUS_OUT_FULL))
        ;
    uint8_t config = inb(KBC_DATA);

    wait_writable();
    outb(KBC_COMMAND, CMD_WRITE_CONFIG);
    wait_writable();
    outb(KBC_DATA, config & ~CONFIG_TRANSLATE);
}
