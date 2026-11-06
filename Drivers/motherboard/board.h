/*
 * board.h - Motherboard clock and pins.
 */
#ifndef BOARD_H
#define BOARD_H

#include <xc.h>
#include <stdint.h>

/* 16 MHz internal oscillator x 3 PLL. */
#define _XTAL_FREQ      48000000UL

/* MSSP I2C pins (fixed by the chip). */
#define I2C_SDA_ANS     ANSELBbits.ANSB0
#define I2C_SDA_TRIS    TRISBbits.TRISB0
#define I2C_SDA_LAT     LATBbits.LATB0
#define I2C_SDA_PORT    PORTBbits.RB0

#define I2C_SCL_ANS     ANSELBbits.ANSB1
#define I2C_SCL_TRIS    TRISBbits.TRISB1
#define I2C_SCL_LAT     LATBbits.LATB1
#define I2C_SCL_PORT    PORTBbits.RB1

/* Scope test pin, driven only in builds with BRINGUP_TICK_PIN defined.
 * PLACEHOLDER PIN: RA1 is not from the design documents. Check the schematic
 * before using it, because the test drives this pin as an output. */
#define BRINGUP_ANS     ANSELAbits.ANSA1
#define BRINGUP_TRIS    TRISAbits.TRISA1
#define BRINGUP_LAT     LATAbits.LATA1

#endif /* BOARD_H */
