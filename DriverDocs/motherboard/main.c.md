# main.c (motherboard)

Source: `Drivers/motherboard/main.c`

## What it does

Starts the motherboard and holds its interrupt handler and main loop. It brings up the clock, tick and I2C hardware, loads the daughter database, registers the chain, and checks for new boards every 100 ms.

**It doesn't poll boards for events or use USB yet**, because those aren't written. A board that drops out isn't noticed until polling exists.

## Inputs and outputs

| Direction | Item | Use |
|---|---|---|
| Input | Timer2 interrupt flag | Calls `tick_isr()` |
| Input | `BRINGUP_TICK_PIN` (build option) | Turns on the scope test |
| Output | I2C bus | Enumeration at boot, then a HELLO every 100 ms |
| Output | `BRINGUP_LAT` (test builds only) | Toggles the test pin on every tick |

`main` takes no arguments and never returns.

## How it works

### Startup order

1. `clock_init()`: first, because all timing depends on it. If it fails, the firmware stops in an empty loop.
2. Sets up the test pin, in test builds only.
3. `tick_init()`: starts the 1 ms tick.
4. `i2c_init()`: sets up the bus hardware.
5. Enables interrupt priorities and global interrupts.
6. `db_init()`: loads the database. This needs the tick and interrupts, because EEPROM writes use a timeout.
7. `chain_enumerate()`: registers the chain.

### Interrupts

| Priority | Source | Handler |
|---|---|---|
| Low | Timer2 | `tick_isr()` |
| High | None yet | USB will go here |

### Main loop

Every `HELLO_INTERVAL_MS` (100 ms):

- If the last enumeration failed, it runs `chain_enumerate()` again.
- Otherwise it runs `chain_append()`, and runs `chain_enumerate()` if that reports an error.

Because the daughter boards may start later than the motherboard, a board missed at boot is picked up by one of these checks.

### Scope test

Build with `BRINGUP_TICK_PIN` defined and probe the pin set in `board.h` (RA1, a placeholder). **Check the schematic first, because this test drives that pin as an output.**

| What the scope shows | Likely meaning |
|---|---|
| 500 Hz square wave, 1 ms high and 1 ms low | The clock and tick are correct |
| About 667 Hz | The PLL is running at 4x instead of 3x |
| Pin stays low | `clock_init()` failed, or the firmware isn't running |

The pin toggles in the main loop, not in the interrupt, so it waits while the loop is busy. Expect a slightly late edge at each 100 ms HELLO. Larger gaps are normal while a new board's database entry is written, and also if the bus has no pull-ups or is stuck, because the firmware keeps retrying.

## Testing

The clock and tick were simulated in MPLAB X before the chain code was added, with `clock_init()` replaced by a version that returns `true`, because the simulator never reports the clock as ready. The test pin toggled at 12,001, 11,999 and 12,001 instruction cycles between edges, against an expected 12,000.

The current `main.c` has been built but not run. The chain code it calls was tested separately, as described in [chain.c](chain.c.md).
