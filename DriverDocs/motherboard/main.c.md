# main.c (motherboard)

Source: `Drivers/motherboard/main.c`

## What it does

Starts the motherboard and holds its interrupt handlers and main loop. The [module manager](module_manager.c.md) does the chain work: registering boards, polling them for events and handling boards that drop out. [usb_app](usb_app.c.md) sends the resulting reports to the PC.

**The scanner for the motherboard's own key switches isn't written yet.** See the [to-do list](../TODO.md).

## Inputs and outputs

| Direction | Item | Use |
|---|---|---|
| Input | USB interrupt | Calls `USBDeviceTasks()` |
| Input | Timer2 interrupt flag | Calls `tick_isr()` |
| Input and output | USB, through `usb_app` | Reports to the PC and its commands |
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
6. `usb_app_init()`: starts USB and connects to the PC. This comes before the chain, so the PC can enumerate the device while boards are still being registered.
7. `module_manager_init()`: loads the database and registers the chain. The I2C and EEPROM timeouts use the tick, so interrupts must already be on.

### Interrupts

| Priority | Source | Handler |
|---|---|---|
| High | USB | `USBDeviceTasks()`, the [MLA stack](mla_usb.md) |
| Low | Timer2 | `tick_isr()` |

USB is high priority because the PC expects quick answers to its control requests. It runs even while the main loop is blocked.

### Main loop

Calls `usb_app_task()` and then `module_manager_task()` on every pass. The key scanner will be added here.

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

The current `main.c`, with USB, has been built with XC8 v3.10 but not run. The code it calls was tested separately, as described in [module_manager.c](module_manager.c.md).
