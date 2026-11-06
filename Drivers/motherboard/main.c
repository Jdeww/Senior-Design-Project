/*
 * main.c - Motherboard startup, interrupt handlers and main loop.
 *
 * So far this brings up the clock, tick and I2C hardware, loads the daughter
 * database, registers the chain and checks for new boards. USB and the polling
 * schedule will be added here as they are written.
 *
 * Build with BRINGUP_TICK_PIN defined to toggle the test pin in board.h on every
 * tick. A scope should show a 500 Hz square wave, 1 ms high and 1 ms low.
 */
#include "board.h"
#include "chain.h"
#include "daughter_db.h"
#include "i2c_master.h"
#include "../common/clock.h"
#include "../common/protocol.h"
#include "../common/system_tick.h"

void __interrupt(low_priority) low_isr(void)
{
    if (PIR1bits.TMR2IF) {
        tick_isr();
    }
}

void main(void)
{
    uint16_t last_hello;
    bool chain_needs_enumerating;
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

    /* These need the tick running. If a write fails, the ID still works until the
     * next reset, and the decision table copes with a database that is out of date. */
    (void)db_init();
    chain_needs_enumerating = !chain_enumerate();
    last_hello = tick_now();

    for (;;) {
        if ((uint16_t)(tick_now() - last_hello) >= HELLO_INTERVAL_MS) {
            last_hello = tick_now();
            if (chain_needs_enumerating || (chain_append() == CHAIN_ERROR)) {
                chain_needs_enumerating = !chain_enumerate();
            }
        }

#ifdef BRINGUP_TICK_PIN
        if ((uint8_t)g_ms != last_tick) {
            last_tick = (uint8_t)g_ms;
            BRINGUP_LAT = !BRINGUP_LAT;
        }
#endif
    }
}
