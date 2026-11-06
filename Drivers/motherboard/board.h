/*
 * board.h - Motherboard clock, pins and system tick.
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

/* Incremented every 1 ms by the Timer2 tick. Must be running, with interrupts
 * enabled, before any I2C call: the bus timeouts depend on it. */
extern volatile uint16_t g_ms;

#endif /* BOARD_H */
