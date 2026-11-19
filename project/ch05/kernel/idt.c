/* idt.c — the interrupt descriptor table.
 *
 * In protected mode the CPU finds interrupt handlers through the IDT: up
 * to 256 8-byte gate descriptors, one per vector, located by the IDTR
 * register (loaded with lidt). A gate holds the handler's address, split
 * in two halves, a code segment selector and a type byte.
 */
#include "interrupts.h"

#define IDT_ENTRIES     256
#define KERNEL_CODE_SEL 0x08
#define NUM_STUBS       48          /* isr.asm: 32 exceptions + 16 IRQs */

/* Type byte: present (bit 7), DPL 0 (bits 5-6), 32-bit interrupt gate
 * (0xE: the CPU clears IF on entry, so handlers start with interrupts off). */
#define GATE_INT32      0x8E

struct idt_gate {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type;
    uint16_t offset_high;
} __attribute__((packed));

/* What lidt loads: the table's size minus one and its address. */
struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

extern const uint32_t isr_stubs[NUM_STUBS];     /* isr.asm */

static struct idt_gate idt[IDT_ENTRIES];

static void idt_set_gate(int vector, uint32_t handler, uint8_t type)
{
    idt[vector].offset_low = handler & 0xFFFF;
    idt[vector].selector = KERNEL_CODE_SEL;
    idt[vector].zero = 0;
    idt[vector].type = type;
    idt[vector].offset_high = handler >> 16;
}

void idt_init(void)
{
    for (int v = 0; v < NUM_STUBS; v++)
        idt_set_gate(v, isr_stubs[v], GATE_INT32);
    /* Vectors 48-255 stay all zero: "not present". Using one is a #GP. */

    struct idt_ptr ptr = { sizeof idt - 1, (uint32_t)idt };
    __asm__ volatile ("lidt %0" : : "m"(ptr));
}
