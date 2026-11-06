# main.c (motherboard)

Source: `Drivers/motherboard/main.c`

## What it does

Starts the motherboard and holds its interrupt handler and main loop. It currently brings up only the clock, tick and I2C hardware. **It doesn't send any I2C traffic or use USB yet**, because the chain enumerator and USB driver aren't written.

## Inputs and outputs

| Direction | Item | Use |
|---|---|---|
| Input | Timer2 interrupt flag | Calls `tick_isr()` |
| Input | `BRINGUP_TICK_PIN` (build option) | Turns on the scope test |
| Output | `BRINGUP_LAT` (test builds only) | Toggles the test pin on every tick |

`main` takes no arguments and never returns.

## How it works

### Startup order

1. `clock_init()`: first, because all timing depends on it. If it fails, the firmware stops in an empty loop.
2. Sets up the test pin, in test builds only.
3. `tick_init()`: starts the 1 ms tick.
4. `i2c_init()`: sets up the bus hardware.
5. Enables interrupt priorities and global interrupts. The I2C timeouts rely on the tick, so this must happen before any I2C traffic.

### Interrupts

| Priority | Source | Handler |
|---|---|---|
| Low | Timer2 | `tick_isr()` |
| High | None yet | USB will go here |

### Main loop

In a normal build the loop is empty. USB servicing, chain enumeration and polling will be added here.

### Scope test

Build with `BRINGUP_TICK_PIN` defined and probe the pin set in `board.h` (RA1, a placeholder). **Check the schematic first, because this test drives that pin as an output.**

| What the scope shows | Likely meaning |
|---|---|
| 500 Hz square wave, 1 ms high and 1 ms low | The clock and tick are correct |
| About 667 Hz | The PLL is running at 4x instead of 3x |
| Pin stays low | `clock_init()` failed, or the firmware isn't running |

The pin toggles in the main loop, not in the interrupt, so small variations in the edges come from the loop's timing and not from the tick.

## Testing

Simulated in MPLAB X with `clock_init()` replaced by a version that returns `true`, because the simulator never reports the clock as ready.

The test pin toggled at 12,001, 11,999 and 12,001 instruction cycles between edges, against an expected 12,000. Real timing still needs the scope test above.
