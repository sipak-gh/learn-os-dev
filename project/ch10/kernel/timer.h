/* timer.h — the PIT on IRQ 0: a tick counter. */
#ifndef SPROUT_TIMER_H
#define SPROUT_TIMER_H

#include <stdint.h>

/* Program PIT channel 0 to interrupt hz times a second and count ticks. */
void timer_init(uint32_t hz);

uint32_t timer_hz(void);

/* Ticks since timer_init. */
uint32_t timer_ticks(void);

/* How many ticks make at least ms milliseconds (ms <= 60000). */
uint32_t timer_ms_to_ticks(uint32_t ms);

#endif
