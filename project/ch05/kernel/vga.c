/* vga.c — a text console on the VGA text buffer.
 *
 * In text mode the screen is an array of 80x25 16-bit cells at physical
 * 0xB8000: the low byte is the character (code page 437), the high byte
 * its colours (bits 0-3 foreground, 4-6 background, 7 blink). Writing a
 * cell changes the screen at once; the hardware cursor is separate and
 * moved through the CRT controller's registers 0x0E and 0x0F.
 */
#include "vga.h"

#include "io.h"
#include "string.h"

#define VGA_COLS    80
#define VGA_ROWS    25
#define CRTC_INDEX  0x3D4
#define CRTC_DATA   0x3D5

static volatile uint16_t *const screen = (volatile uint16_t *)0xB8000;
static int row, col;
static uint8_t color = VGA_LIGHT_GREY | VGA_BLACK << 4;

static uint16_t cell(char c)
{
    return (uint16_t)(unsigned char)c | (uint16_t)color << 8;
}

static void move_cursor(void)
{
    uint16_t pos = (uint16_t)(row * VGA_COLS + col);
    outb(CRTC_INDEX, 0x0E);
    outb(CRTC_DATA, pos >> 8);
    outb(CRTC_INDEX, 0x0F);
    outb(CRTC_DATA, pos & 0xFF);
}

void vga_init(void)
{
    for (int i = 0; i < VGA_COLS * VGA_ROWS; i++)
        screen[i] = cell(' ');
    row = col = 0;
    move_cursor();
}

void vga_set_color(enum vga_color fg, enum vga_color bg)
{
    color = (uint8_t)(fg | bg << 4);
}

/* Move every row up by one and blank the last. */
static void scroll(void)
{
    for (int i = 0; i < VGA_COLS * (VGA_ROWS - 1); i++)
        screen[i] = screen[i + VGA_COLS];
    for (int i = VGA_COLS * (VGA_ROWS - 1); i < VGA_COLS * VGA_ROWS; i++)
        screen[i] = cell(' ');
    row = VGA_ROWS - 1;
}

void vga_putc(char c)
{
    switch (c) {
    case '\n':
        col = 0;
        row++;
        break;
    case '\r':
        col = 0;
        break;
    case '\b':
        if (col > 0)
            col--;
        break;
    case '\t':
        col = (col + 8) & ~7;
        break;
    default:
        screen[row * VGA_COLS + col] = cell(c);
        col++;
    }
    if (col >= VGA_COLS) {
        col = 0;
        row++;
    }
    if (row >= VGA_ROWS)
        scroll();
    move_cursor();
}
