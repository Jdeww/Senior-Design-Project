/*
 * system_tick.h - 1 ms time base from Timer2, used on both boards.
 *
 * Wiring it into the firmware:
 *   - call tick_init() at boot, then set RCONbits.IPEN and enable interrupts
 *   - in the low-priority ISR: if (PIR1bits.TMR2IF) tick_isr();
 *
 * The counter wraps about every 65.5 seconds. Measure time by subtracting, which
 * still gives the right answer across a wrap:
 *   if ((uint16_t)(tick_now() - last_poll) >= POLL_INTERVAL_MS) { ... }
 */
#ifndef SYSTEM_TICK_H
#define SYSTEM_TICK_H

#include <stdint.h>

/* Milliseconds since boot. Written only by tick_isr(). In the main loop, use
 * tick_now() to read all of it, or cast to uint8_t to read just the low byte. */
extern volatile uint16_t g_ms;

void tick_init(void);
void tick_isr(void);

/* Main loop only. Returns g_ms, safe against a tick arriving during the read. */
uint16_t tick_now(void);

#endif /* SYSTEM_TICK_H */
