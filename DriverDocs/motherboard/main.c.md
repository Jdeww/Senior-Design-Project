# main.c (motherboard)

Source: `Drivers/motherboard/main.c`

## What it does

Starts the motherboard and holds its interrupt handler and main loop. The [module manager](module_manager.c.md) does the chain work: registering boards, polling them for events and handling boards that drop out.

**USB isn't written yet, so nothing drains the report queue.** After 40 reports it stays full. Boards are still polled and dropped boards are still noticed, but events wait on the daughter boards.

## Inputs and outputs

| Direction | Item | Use |
|---|---|---|
| Input | Timer2 interrupt flag | Calls `tick_isr()` |
| Input | `BRINGUP_TICK_PIN` (build option) | Turns on the scope test |
| Output | I2C bus, through the module manager | Enumeration at boot, polling every 5 ms and a HELLO every 100 ms |
| Output | `BRINGUP_LAT` (test builds only) | Toggles the test pin on every tick |

`main` takes no arguments and never returns.

## How it works

### Startup order

1. `clock_init()`: first, because all timing depends on it. If it fails, the firmware stops in an empty loop.
2. Sets up the test pin, in test builds only.
3. `tick_init()`: starts the 1 ms tick.
4. `i2c_init()`: sets up the bus hardware.
5. Enables interrupt priorities and global interrupts.
6. `module_manager_init()`: loads the database and registers the chain. The I2C and EEPROM timeouts use the tick, so interrupts must already be on.

### Interrupts

| Priority | Source | Handler |
|---|---|---|
| Low | Timer2 | `tick_isr()` |
| High | None yet | USB will go here |

### Main loop

Calls `module_manager_task()` on every pass. USB servicing will be added here.

### Scope test

Build with `BRINGUP_TICK_PIN` defined and probe the pin set in `board.h` (RA1, a placeholder). **Check the schematic first, because this test drives that pin as an output.**

| What the scope shows | Likely meaning |
|---|---|
| 500 Hz square wave, 1 ms high and 1 ms low | The clock and tick are correct |
| About 667 Hz | The PLL is running at 4x instead of 3x |
| Pin stays low | `clock_init()` failed, or the firmware isn't running |

The pin toggles in the main loop, not in the interrupt, so it waits while the loop is busy. Expect slightly late edges from polling, especially with several boards, and larger gaps while a new board's database entry is written or if the bus has no pull-ups or is stuck.

## Testing

The clock and tick were simulated in MPLAB X before the chain code was added, with `clock_init()` replaced by a version that returns `true`, because the simulator never reports the clock as ready. The test pin toggled at 12,001, 11,999 and 12,001 instruction cycles between edges, against an expected 12,000.

The current `main.c` has been built but not run. The code it calls was tested separately, as described in [module_manager.c](module_manager.c.md).
