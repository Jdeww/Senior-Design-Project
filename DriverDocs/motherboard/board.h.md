# board.h (motherboard)

Source: `Drivers/motherboard/board.h`

## What it does

Holds the motherboard's hardware details in one place: its clock speed and which pins carry I2C. Other motherboard files use these names instead of naming registers or pins directly.

## Inputs and outputs

This file has no functions. It provides these names:

| Name | Value | Purpose |
|---|---|---|
| `_XTAL_FREQ` | 48000000 | CPU clock in Hz. Used for the I2C clock calculation and required by `__delay_us()`. |
| `I2C_SDA_ANS`, `I2C_SCL_ANS` | `ANSB0`, `ANSB1` | Analog/digital selection for the I2C pins |
| `I2C_SDA_TRIS`, `I2C_SCL_TRIS` | `TRISB0`, `TRISB1` | Pin direction: 1 = input (released), 0 = output |
| `I2C_SDA_LAT`, `I2C_SCL_LAT` | `LATB0`, `LATB1` | The level a pin drives when it is an output |
| `I2C_SDA_PORT`, `I2C_SCL_PORT` | `RB0`, `RB1` | The level actually on the pin |

## How it works

- **Clock:** 48 MHz comes from the 16 MHz internal oscillator with the 3x PLL. This file only states the frequency; it doesn't configure the oscillator. That is left to startup code, which isn't written yet.
- **Timer settings:** [system_tick.c](../common/system_tick.c.md) reads `_XTAL_FREQ` from this file to choose its Timer2 settings.
- **Pins:** SDA is RB0 and SCL is RB1, which are the chip's fixed MSSP pins.
