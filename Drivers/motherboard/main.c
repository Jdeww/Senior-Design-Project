/*
 * main.c - Motherboard startup, interrupt handlers and main loop.
 *
 * USB is not written yet, so nothing drains the report queue. Once it fills, boards
 * are still polled but their events wait on the daughter boards.
 *
 * Build with BRINGUP_TICK_PIN defined to toggle the test pin in board.h on every
 * tick. A scope should show a 500 Hz square wave, 1 ms high and 1 ms low.
 */
#include "board.h"
#include "i2c_master.h"
#include "module_manager.h"
#include "../common/clock.h"
#include "../common/system_tick.h"

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
    i2c_init();

    RCONbits.IPEN = 1;
    INTCONbits.GIEH = 1;
    INTCONbits.GIEL = 1;

    /* This uses the tick for I2C and EEPROM timeouts, so interrupts must be on. */
    module_manager_init();

    for (;;) {
        module_manager_task();

#ifdef BRINGUP_TICK_PIN
        if ((uint8_t)g_ms != last_tick) {
            last_tick = (uint8_t)g_ms;
            BRINGUP_LAT = !BRINGUP_LAT;
        }
#endif
    }
}
