/*
 * clock.h - Configuration bits and oscillator startup, used on both boards.
 *
 * Call clock_init() first thing in main. Until it returns, the chip is running
 * slower than _XTAL_FREQ, so every timing in the firmware is wrong.
 */
#ifndef CLOCK_H
#define CLOCK_H

#include <stdbool.h>

/* Returns true once the clock is stable at _XTAL_FREQ, or false if it never
 * settled. Without a stable clock, USB and the tick cannot be trusted, so the
 * caller should stop rather than carry on. */
bool clock_init(void);

#endif /* CLOCK_H */
