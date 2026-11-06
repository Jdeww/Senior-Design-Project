# clock.c

Source: `Drivers/common/clock.c`
Interface: [clock.h](clock.h.md)

## What it does

Sets the chip's configuration bits and brings the oscillator up to the board's speed: 48 MHz on the motherboard and 16 MHz on daughter boards. The same file builds for both, choosing its settings from `_XTAL_FREQ`.

## Inputs and outputs

The function's result is described in [clock.h](clock.h.md). This file also interacts with:

| Direction | Item | Use |
|---|---|---|
| Input | `_XTAL_FREQ` from `board.h` | Selects the settings at compile time |
| Input | `OSCCON2.PLLRDY` (motherboard) | Set by the hardware when the PLL has locked |
| Input | `OSCCON.HFIOFS` (daughter) | Set by the hardware when the oscillator is stable |
| Output | Configuration bits | Fixed settings stored in the programmed firmware |
| Output | `OSCCON`, `OSCTUNE` | Select 16 MHz and the 3x PLL |
| Output | `ACTCON` (motherboard) | Turns on Active Clock Tuning from USB frames |

Because `board.h` differs by board, each build's include path must contain that board's folder. This is why the file includes `board.h` with no folder in front of it.

## How it works

### Configuration bits

These are read by the chip at reset, before any code runs.

| Setting | Value | Why |
|---|---|---|
| `FOSC` | `INTOSCIO` | Internal oscillator, with no crystal |
| `PCLKEN`, `FCMEN`, `IESO` | `OFF` | Only needed with an external oscillator |
| `CPUDIV` | `NOCLKDIV` | The CPU runs at the full system clock |
| `nPWRTEN` | `ON` | Gives the supply time to settle at power-up |
| `BOREN`, `BORV` | `SBORDIS`, 2.85 V | Resets on low voltage, which protects EEPROM writes |
| `nLPBOR` | `OFF` | The standard brown-out reset above is already on |
| `WDTEN` | `OFF` | No watchdog handling is written yet |
| `PBADEN` | `OFF` | PORTB starts digital |
| `MCLRE` | `ON` | Keeps the reset pin; the board needs a pull-up on it |
| `STVREN` | `ON` | Resets on a stack overflow instead of running on corrupted |
| `LVP` | `OFF` | Frees RB5 as I/O, but programming then needs a programmer that supplies high voltage on MCLR |
| `XINST` | `OFF` | XC8 doesn't support the extended instruction set |

| Board setting | Motherboard | Daughter |
|---|---|---|
| `CFGPLLEN` | `ON` | `OFF` |
| `PLLSEL` | `PLL3X` | `PLL3X` (not used, since the PLL is off) |
| `LS48MHZ` | `SYS48X8` | `SYS48X8` (not used, since there is no USB) |

`PLLSEL` must be set explicitly because its default is 4x, which would run the motherboard at 64 MHz.

### `clock_init()` on the motherboard

1. Writes `OSCTUNE = 0x80` to select the 3x PLL.
2. Writes `OSCCON = 0x70` to select the 16 MHz internal oscillator.
3. Waits for `PLLRDY`, returning `false` if the wait runs out.
4. Writes `ACTCON = 0x90` to turn on Active Clock Tuning from USB Start-of-Frame packets. This is what keeps the clock within the USB tolerance without a crystal. Until USB is running there are no frames, so no trimming happens.

### `clock_init()` on a daughter board

1. Writes `OSCCON = 0x70` to select the 16 MHz internal oscillator.
2. Waits for `HFIOFS`, returning `false` if the wait runs out.

### The wait limit

The wait counts down from 65,535. In the simulator, the full wait took about 721,000 instruction cycles, or about 60 ms at 48 MHz and 180 ms at 16 MHz.

## Testing

| Check | Method | Result |
|---|---|---|
| Configuration bytes | Read from the generated `.hex` file | Match the settings above: `CONFIG1L` is `0x23` on the motherboard and `0x21` on daughters |
| Register writes | MPLAB X simulator | `OSCCON` read `0x70` and `OSCTUNE` read `0x80` |
| Successful startup | Not tested | The simulator never set `PLLRDY` or `HFIOFS`, so `clock_init()` returned `false` there |

Whether the clock actually starts, and at what speed, must be checked on real hardware. See the scope test in the [motherboard main.c page](../motherboard/main.c.md).

The motherboard's `OSCTUNE`, `OSCCON` and `ACTCON` sequence is based on memory of Microchip's USB demo code. It should be checked against the PIC18LF25K50 datasheet.
