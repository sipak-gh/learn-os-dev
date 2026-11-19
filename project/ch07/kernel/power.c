/* power.c — reboot through the keyboard controller; power off in QEMU.
 *
 * The 8042 keyboard controller's output port bit 0 is wired to the CPU's
 * reset line (the same chip gates A20, Chapter 3): command 0xFE pulses it.
 *
 * Powering off a PC is an ACPI job: find the FADT and the \_S5 sleep type
 * in the ACPI tables, write it to the PM1a control register. That needs
 * an AML parser. QEMU's PC machine has its PM1a control register at port
 * 0x604, and we just write the "sleep enable" bit with sleep type 0 there,
 * which QEMU takes as "off". This works in QEMU (2.0 and later) only.
 */
#include "power.h"

#include "io.h"

#define KBC_STATUS      0x64
#define KBC_COMMAND     0x64
#define KBC_INPUT_FULL  0x02        /* the controller hasn't read the last byte */
#define KBC_PULSE_RESET 0xFE

#define QEMU_PM1A_CNT   0x604
#define PM1_SLP_EN      0x2000      /* bit 13 */

void reboot(void)
{
    cli();
    while (inb(KBC_STATUS) & KBC_INPUT_FULL)
        ;
    outb(KBC_COMMAND, KBC_PULSE_RESET);
    for (;;)
        hlt();                      /* the reset takes a moment */
}

void poweroff(void)
{
    cli();
    outw(QEMU_PM1A_CNT, PM1_SLP_EN);
    /* QEMU exits a moment later (the CPU may run a few more instructions
     * first, so print nothing here). Elsewhere, this is where we stay. */
    for (;;)
        hlt();
}
