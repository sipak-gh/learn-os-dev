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

/* Control registers (Chapter 7). The "memory" clobber stops the compiler
 * from moving memory accesses across a change of address space. */
static inline uint32_t read_cr0(void)
{
    uint32_t value;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(value));
    return value;
}

static inline void write_cr0(uint32_t value)
{
    __asm__ volatile ("mov %0, %%cr0" : : "r"(value) : "memory");
}

static inline uint32_t read_cr2(void)
{
    uint32_t value;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(value));
    return value;
}

static inline uint32_t read_cr3(void)
{
    uint32_t value;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(value));
    return value;
}

static inline void write_cr3(uint32_t value)
{
    __asm__ volatile ("mov %0, %%cr3" : : "r"(value) : "memory");
}

/* Forget the TLB's cached translation for the page holding addr. */
static inline void invlpg(uint32_t addr)
{
    __asm__ volatile ("invlpg (%0)" : : "r"(addr) : "memory");
}

#endif
