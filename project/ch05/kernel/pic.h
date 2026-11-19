/* pic.h — the two 8259A programmable interrupt controllers. */
#ifndef SPROUT_PIC_H
#define SPROUT_PIC_H

#include <stdbool.h>
#include <stdint.h>

/* Move IRQ 0-15 to vectors base..base+15 and mask all of them. */
void pic_init(uint8_t base);

/* Tell the PIC(s) that IRQ irq has been handled. */
void pic_eoi(int irq);

void pic_unmask(int irq);
void pic_mask(int irq);

/* True if IRQ 7 or 15 fired without a real request behind it. */
bool pic_is_spurious(int irq);

#endif
