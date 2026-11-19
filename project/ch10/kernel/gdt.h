/* gdt.h — the kernel's GDT: ring 0 and ring 3 segments, and the TSS. */
#ifndef SPROUT_GDT_H
#define SPROUT_GDT_H

#include <stdint.h>

/* Selectors: index * 8, plus the requested privilege level (RPL) in the
 * low two bits. User selectors carry RPL 3. */
#define KERNEL_CS   0x08
#define KERNEL_DS   0x10
#define USER_CS     (0x18 | 3)
#define USER_DS     (0x20 | 3)
#define TSS_SEL     0x28

/* Replace stage 2's GDT with the kernel's and load the task register. */
void gdt_init(void);

/* The stack the CPU switches to when an interrupt or a system call
 * arrives while ring 3 code runs. The scheduler sets it on every switch:
 * each task has its own kernel stack. */
void tss_set_kernel_stack(uint32_t esp0);

#endif
