/*
 * clock.c - See clock.h.
 *
 * board.h supplies _XTAL_FREQ, so each board's include path must contain its own
 * board folder. This file builds for both boards, so board.h is the one include
 * written without a folder.
 */
#include "board.h"
#include "common/clock.h"

/* Settings shared by both boards. */
#pragma config FOSC = INTOSCIO      /* internal oscillator; RA6 and RA7 stay I/O */
#pragma config PCLKEN = OFF         /* no crystal, so no primary oscillator      */
#pragma config FCMEN = OFF
#pragma config IESO = OFF
#pragma config CPUDIV = NOCLKDIV
#pragma config nPWRTEN = ON         /* let the supply settle at power-up         */
#pragma config BOREN = SBORDIS      /* brown-out reset protects EEPROM writes    */
#pragma config BORV = 285
#pragma config nLPBOR = OFF
#pragma config WDTEN = OFF
#pragma config PBADEN = OFF         /* PORTB starts digital                      */
#pragma config MCLRE = ON           /* needs a pull-up on the MCLR pin           */
#pragma config STVREN = ON
#pragma config LVP = OFF            /* RB5 stays I/O; program with MCLR high volts */
#pragma config XINST = OFF          /* XC8 does not support the extended set     */

#if _XTAL_FREQ == 48000000UL
    /* Motherboard: 16 MHz x 3 = 48 MHz. PLLSEL defaults to 4x, so set it. */
    #pragma config PLLSEL = PLL3X
    #pragma config CFGPLLEN = ON
    #pragma config LS48MHZ = SYS48X8

    #define OSCTUNE_3X_PLL      0x80u   /* SPLLMULT = 1                         */
    #define ACTCON_USB_SOF      0x90u   /* ACTEN = 1, ACTSRC = USB Start-of-Frame */
#elif _XTAL_FREQ == 16000000UL
    /* Daughter board: 16 MHz with no PLL, because it has no USB. */
    #pragma config PLLSEL = PLL3X
    #pragma config CFGPLLEN = OFF
    #pragma config LS48MHZ = SYS48X8
#else
    #error "clock: no settings for this clock speed"
#endif

#define OSCCON_16MHZ            0x70u   /* IRCF = 111, SCS = 00 (use FOSC above) */
#define CLOCK_WAIT_LIMIT        0xFFFFu

bool clock_init(void)
{
    uint16_t wait = CLOCK_WAIT_LIMIT;

#if _XTAL_FREQ == 48000000UL
    OSCTUNE = OSCTUNE_3X_PLL;
    OSCCON = OSCCON_16MHZ;

    while (!OSCCON2bits.PLLRDY) {
        wait--;
        if (wait == 0u) {
            return false;
        }
    }

    /* Trims the oscillator against the PC's 1 ms USB frames, which is what keeps
     * it within the USB tolerance with no crystal. Until USB is running there
     * are no frames, so no trimming happens yet. */
    ACTCON = ACTCON_USB_SOF;
#else
    OSCCON = OSCCON_16MHZ;

    while (!OSCCONbits.HFIOFS) {
        wait--;
        if (wait == 0u) {
            return false;
        }
    }
#endif

    return true;
}
