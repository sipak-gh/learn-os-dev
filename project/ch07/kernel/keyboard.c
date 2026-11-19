/* keyboard.c — PS/2 keyboard: scancodes in, characters out.
 *
 * Each key press and release makes the keyboard send scancodes to the
 * 8042 controller, which raises IRQ 1; we read them from port 0x60. The
 * keyboard speaks scancode set 2, but the 8042 translates to the older
 * set 1 (the IBM PC/XT's) unless told otherwise, and the BIOS leaves
 * translation on. In set 1 a press sends the key's code and a release
 * sends the same code with bit 7 set. Some keys added later (arrows, the
 * right Ctrl) send 0xE0 first.
 *
 * Scancodes name keys, not characters: 0x1E is "the key left of S", which
 * says 'a' on a US keyboard. The tables below are the US layout.
 */
#include "keyboard.h"

#include <stdbool.h>
#include <stdint.h>

#include "input.h"
#include "interrupts.h"
#include "io.h"
#include "pic.h"

#define KBD_DATA        0x60
#define KBD_STATUS      0x64
#define STATUS_OUTPUT_FULL 0x01     /* a byte is waiting in port 0x60 */

#define SC_RELEASE      0x80
#define SC_EXTENDED     0xE0
#define SC_LSHIFT       0x2A
#define SC_RSHIFT       0x36
#define SC_CAPSLOCK     0x3A
#define SC_ENTER        0x1C

/* Set 1 make codes 0x00-0x39 to characters; 0 = no character. */
static const char normal[0x3A] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,  'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,  '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0,   '*', 0,   ' ',
};

static const char shifted[0x3A] = {
    0,   27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', '\t',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,  'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,   '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ',
};

static bool shift_left, shift_right, caps_lock, extended;

static char translate(uint8_t code)
{
    bool shift = shift_left || shift_right;
    char c = shift ? shifted[code] : normal[code];

    /* Caps Lock affects letters only, and Shift undoes it. */
    char lower = normal[code];
    if (caps_lock && lower >= 'a' && lower <= 'z')
        c = shift ? lower : (char)(lower - 'a' + 'A');
    return c;
}

static void keyboard_irq(struct regs *r)
{
    (void)r;
    uint8_t sc = inb(KBD_DATA);

    if (sc == SC_EXTENDED) {
        extended = true;
        return;
    }
    if (extended) {                     /* the second byte of an E0 pair */
        extended = false;
        if (sc == SC_ENTER)             /* keypad Enter */
            input_put('\n');
        return;                         /* arrows, right Ctrl...: ignored */
    }

    bool released = sc & SC_RELEASE;
    uint8_t code = sc & ~SC_RELEASE;

    if (code == SC_LSHIFT) {
        shift_left = !released;
    } else if (code == SC_RSHIFT) {
        shift_right = !released;
    } else if (code == SC_CAPSLOCK) {
        if (!released)
            caps_lock = !caps_lock;
    } else if (!released && code < sizeof normal) {
        char c = translate(code);
        if (c != 0)
            input_put(c);
    }
}

void keyboard_init(void)
{
    /* Throw away anything typed before we were ready. */
    while (inb(KBD_STATUS) & STATUS_OUTPUT_FULL)
        inb(KBD_DATA);

    interrupt_register(IRQ_BASE + IRQ_KEYBOARD, keyboard_irq);
    pic_unmask(IRQ_KEYBOARD);
}
