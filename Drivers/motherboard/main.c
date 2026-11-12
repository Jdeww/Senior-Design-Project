/*
 * main.c - Motherboard startup, interrupt handlers and main loop.
 *
 * The scanner for the motherboard's own key switches is not written yet.
 *
 * Build with BRINGUP_TICK_PIN defined to toggle the test pin in board.h on every
 * tick. A scope should show a 500 Hz square wave, 1 ms high and 1 ms low.
 */
#include "motherboard/board.h"
#include "motherboard/i2c_master.h"
#include "motherboard/module_manager.h"
#include "motherboard/usb_app.h"
#include "common/clock.h"
#include "common/system_tick.h"
#include "usb_config.h"
#include "usb.h"

/* USB is the only high-priority source. The stack checks its own flags. */
void __interrupt(high_priority) high_isr(void)
{
    USBDeviceTasks();
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
    i2c_init();

    RCONbits.IPEN = 1;
    INTCONbits.GIEH = 1;
    INTCONbits.GIEL = 1;

    /* Before the chain, so the PC can enumerate the device while boards are
     * being registered. From here on the USB interrupt answers the PC even
     * while the main loop is blocked. */
    usb_app_init();

    /* This uses the tick for I2C and EEPROM timeouts, so interrupts must be on. */
    module_manager_init();

    for (;;) {
        usb_app_task();
        module_manager_task();

#ifdef BRINGUP_TICK_PIN
        if ((uint8_t)g_ms != last_tick) {
            last_tick = (uint8_t)g_ms;
            BRINGUP_LAT = !BRINGUP_LAT;
        }
#endif
    }
}
