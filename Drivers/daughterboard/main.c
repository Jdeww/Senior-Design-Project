/*
 * main.c - Daughter board startup, interrupt handlers and main loop.
 *
 * The board joins the I2C bus, but input detection and the input drivers are not
 * written yet, so it reports INPUT_TYPE_NONE and never queues an event.
 *
 * Build with BRINGUP_TICK_PIN defined to toggle the test pin in board.h on every
 * tick. A scope should show a 500 Hz square wave, 1 ms high and 1 ms low.
 */
#include "board.h"
#include "i2c_slave.h"
#include "../common/clock.h"
#include "../common/protocol.h"
#include "../common/system_tick.h"

void __interrupt(high_priority) high_isr(void)
{
    if (PIR1bits.SSPIF) {
        i2c_slave_isr();
    }
}

void __interrupt(low_priority) low_isr(void)
{
    if (PIR1bits.TMR2IF) {
        tick_isr();
    }
}

void main(void)
{
#ifdef BRINGUP_TICK_PIN
    uint8_t last_tick = 0u;
#endif

    if (!clock_init()) {
        /* Nothing that follows can work at the wrong clock speed. */
        for (;;) {
        }
    }

#ifdef BRINGUP_TICK_PIN
    BRINGUP_ANS = 0;
    BRINGUP_LAT = 0;
    BRINGUP_TRIS = 0;
#endif

    tick_init();
    i2c_slave_init(INPUT_TYPE_NONE, 0u);

    RCONbits.IPEN = 1;
    INTCONbits.GIEH = 1;
    INTCONbits.GIEL = 1;

    for (;;) {
        i2c_slave_task();

#ifdef BRINGUP_TICK_PIN
        if ((uint8_t)g_ms != last_tick) {
            last_tick = (uint8_t)g_ms;
            BRINGUP_LAT = !BRINGUP_LAT;
        }
#endif
    }
}
