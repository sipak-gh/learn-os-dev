/* timer.h — the PIT on IRQ 0: a tick counter. */
#ifndef SPROUT_TIMER_H
#define SPROUT_TIMER_H

#include <stdint.h>

/* Program PIT channel 0 to interrupt hz times a second and count ticks. */
void timer_init(uint32_t hz);

uint32_t timer_hz(void);

/* Ticks since timer_init. */
uint32_t timer_ticks(void);

/* Wait (with hlt) until n more ticks have happened. Needs interrupts on. */
void timer_sleep(uint32_t n);

#endif
