/*
 * system_tick.c - See system_tick.h.
 *
 * Timer2 counts instruction cycles (Fosc / 4):
 *   tick rate = Fosc / 4 / prescale / (PR2 + 1) / postscale
 *
 * board.h supplies _XTAL_FREQ, so each board's include path must contain its own
 * board folder.
 */
#include "board.h"
#include "system_tick.h"
#include "protocol.h"

#define TICK_PR2                249u    /* 250 counts per period */

#if _XTAL_FREQ == 48000000UL
    /* Motherboard: 12 MHz / 16 / 250 / 3 = 1000 Hz */
    #define TICK_PRESCALE       16UL
    #define TICK_POSTSCALE      3UL
    #define TICK_T2CKPS         2u      /* 1x = 1:16 */
#elif _XTAL_FREQ == 16000000UL
    /* Daughter board: 4 MHz / 4 / 250 / 4 = 1000 Hz */
    #define TICK_PRESCALE       4UL
    #define TICK_POSTSCALE      4UL
    #define TICK_T2CKPS         1u      /* 01 = 1:4 */
#else
    #error "system_tick: no Timer2 settings for this clock speed"
#endif

#define TICK_T2OUTPS            ((uint8_t)(TICK_POSTSCALE - 1UL))   /* value + 1 */

#if (_XTAL_FREQ / 4UL / TICK_PRESCALE / (TICK_PR2 + 1UL) / TICK_POSTSCALE) != (1000UL / TICK_MS)
    #error "system_tick: Timer2 settings do not give TICK_MS"
#endif

volatile uint16_t g_ms;

void tick_init(void)
{
    T2CON = 0;
    TMR2 = 0;
    PR2 = TICK_PR2;
    T2CONbits.T2CKPS = TICK_T2CKPS;
    T2CONbits.T2OUTPS = TICK_T2OUTPS;

    PIR1bits.TMR2IF = 0;
    IPR1bits.TMR2IP = 0;    /* low priority */
    PIE1bits.TMR2IE = 1;

    T2CONbits.TMR2ON = 1;
}

void tick_isr(void)
{
    PIR1bits.TMR2IF = 0;
    g_ms++;
}

uint16_t tick_now(void)
{
    uint16_t first;
    uint16_t second;

    /* g_ms takes two reads on this 8-bit chip. A tick between them could pair
     * the old low byte with the new high byte, so read until two reads match.
     * This avoids switching interrupts off, which would add jitter to the tick. */
    do {
        first = g_ms;
        second = g_ms;
    } while (first != second);

    return first;
}
