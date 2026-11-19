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

/* The "memory" clobbers make these compiler barriers too: cli/sti now
 * guard data shared with interrupt handlers and other tasks (Chapter 8),
 * and the compiler must not move loads and stores across them. */
static inline void cli(void) { __asm__ volatile ("cli" ::: "memory"); }
static inline void sti(void) { __asm__ volatile ("sti" ::: "memory"); }
static inline void hlt(void) { __asm__ volatile ("hlt" ::: "memory"); }

#define EFLAGS_IF (1u << 9)     /* interrupts enabled */

/* Disable interrupts and return the old EFLAGS, for irq_restore. Pairs
 * nest: an inner pair leaves interrupts the way the outer one found them. */
static inline uint32_t irq_save(void)
{
    uint32_t flags;
    __asm__ volatile ("pushf; pop %0; cli" : "=r"(flags) :: "memory");
    return flags;
}

static inline void irq_restore(uint32_t flags)
{
    if (flags & EFLAGS_IF)
        sti();
}

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
