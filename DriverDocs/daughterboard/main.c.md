# main.c (daughter board)

Source: `Drivers/daughterboard/main.c`

## What it does

Starts the daughter board and holds its interrupt handlers and main loop. The board joins the I2C bus and can answer the handshake, but input detection and the input drivers aren't written yet. It therefore reports `INPUT_TYPE_NONE` and never queues an event.

## Inputs and outputs

| Direction | Item | Use |
|---|---|---|
| Input | MSSP interrupt flag | Calls `i2c_slave_isr()` |
| Input | Timer2 interrupt flag | Calls `tick_isr()` |
| Input | `BRINGUP_TICK_PIN` (build option) | Turns on the scope test |
| Output | `BRINGUP_LAT` (test builds only) | Toggles the test pin on every tick |

`main` takes no arguments and never returns. The I2C slave driver handles the bus and gate pins.

## How it works

### Startup order

1. `clock_init()`: first, because all timing depends on it. If it fails, the firmware stops in an empty loop.
2. Sets up the test pin, in test builds only.
3. `tick_init()`: starts the 1 ms tick.
4. `i2c_slave_init(INPUT_TYPE_NONE, 0)`: joins the bus at 0x2F with the gate open.
5. Enables interrupt priorities and global interrupts.

**Every build drives the gate pin set in `board.h` as an output. RB2 is a placeholder, so change it to match the schematic before flashing a real board.**

### Interrupts

| Priority | Source | Handler |
|---|---|---|
| High | MSSP (I2C) | `i2c_slave_isr()` |
| Low | Timer2 | `tick_isr()` |

### Main loop

Calls `i2c_slave_task()` on every pass, which prepares HELLO replies and saves new IDs. The input drivers will be added here.

### Scope test

This works the same way as on the [motherboard](../motherboard/main.c.md): build with `BRINGUP_TICK_PIN` defined and look for a 500 Hz square wave on the test pin (RA1, a placeholder). **Check the schematic first, because this test drives that pin as an output.**

## Testing

Simulated in MPLAB X with `clock_init()` replaced by a version that returns `true`, because the simulator never reports the clock as ready.

The test pin toggled at 3,991, 3,989 and 4,027 instruction cycles between edges, against an expected 4,000. The variation is up to about 10 µs, which comes from the toggle waiting for its turn in the main loop. The I2C paths did not run in this test.
