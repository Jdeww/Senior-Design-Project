# board.h (daughter board)

Source: `Drivers/daughterboard/board.h`

## What it does

Holds the daughter board's hardware details in one place: its clock speed, its I2C pins and the chain gate pin. Other daughter files use these names instead of naming pins directly.

## Inputs and outputs

This file has no functions. It provides these names:

| Name | Value | Purpose |
|---|---|---|
| `_XTAL_FREQ` | 16000000 | CPU clock in Hz |
| `I2C_SDA_ANS`, `I2C_SCL_ANS` | `ANSB0`, `ANSB1` | Analog/digital selection for the I2C pins |
| `I2C_SDA_TRIS`, `I2C_SCL_TRIS` | `TRISB0`, `TRISB1` | Pin direction for the I2C pins |
| `GATE_ANS` | `ANSB2` | Analog/digital selection for the gate pin |
| `GATE_TRIS` | `TRISB2` | Gate pin direction |
| `GATE_LAT` | `LATB2` | Gate pin level: 1 = closed, 0 = open |

## How it works

- **Clock:** 16 MHz from the internal oscillator with no PLL, since daughters don't use USB. This file only states the frequency; startup code, which isn't written yet, must configure the oscillator.
- **I2C pins:** SDA is RB0 and SCL is RB1, which are the chip's fixed MSSP pins. There are no latch or port names here because slave mode doesn't control the pins directly.
- **Chain gate:** the pin drives the enable of the switch on the downstream SDA and SCL lines. High closes the gate and connects the next board; low opens it and isolates that board.

> **Placeholder:** RB2 for the gate is not from the design documents. Change these three definitions to match the schematic.
