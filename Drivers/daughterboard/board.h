/*
 * board.h - Daughter board clock and pins.
 */
#ifndef BOARD_H
#define BOARD_H

#include <xc.h>
#include <stdint.h>

/* 16 MHz internal oscillator, no PLL. */
#define _XTAL_FREQ      16000000UL

/* MSSP I2C pins (fixed by the chip). */
#define I2C_SDA_ANS     ANSELBbits.ANSB0
#define I2C_SDA_TRIS    TRISBbits.TRISB0
#define I2C_SCL_ANS     ANSELBbits.ANSB1
#define I2C_SCL_TRIS    TRISBbits.TRISB1

/* Chain gate enable: high = gate closed (next board connected), low = open.
 * PLACEHOLDER PIN: RB2 is not from the design documents. Change it to match
 * the schematic. */
#define GATE_ANS        ANSELBbits.ANSB2
#define GATE_TRIS       TRISBbits.TRISB2
#define GATE_LAT        LATBbits.LATB2

#endif /* BOARD_H */
