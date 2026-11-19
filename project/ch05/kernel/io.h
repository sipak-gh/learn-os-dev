/* io.h — x86 port I/O and a few single instructions, as inline functions.
 *
 * C has no way to say "in" or "out": the I/O port space is separate from
 * memory. GCC's extended asm lets us write the one instruction and tell the
 * compiler which registers it uses ("a" = eax/ax/al, "Nd" = dx or an
 * 8-bit constant).
 */
#ifndef SPROUT_IO_H
#define SPROUT_IO_H

#include <stdint.h>

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outw(uint16_t port, uint16_t value)
{
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint16_t inw(uint16_t port)
{
    uint16_t value;
    __asm__ volatile ("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void cli(void) { __asm__ volatile ("cli"); }
static inline void sti(void) { __asm__ volatile ("sti"); }
static inline void hlt(void) { __asm__ volatile ("hlt"); }

#endif
