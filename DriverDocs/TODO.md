# Driver To-Do List

What's left to do on the macro pad firmware, in the suggested order. Check items off as they're finished.

## 1. Before the first flash

These need the schematic or datasheet, and getting them wrong could stop a board working or drive the wrong pin.

- [ ] **Set the gate pin.** RB2 in `daughterboard/board.h` is a placeholder, and every daughter build drives it as an output. Change it to match the schematic.
- [ ] **Check the test pin.** RA1 in both `board.h` files is a placeholder, and scope-test builds drive it as an output. Confirm it's safe or choose another pin.
- [ ] **Check the programmer against `LVP`.** `LVP` is off in `common/clock.c`, so programming needs high voltage on MCLR. A PICkit should handle this. As far as we know an MPLAB Snap can't, so with a Snap, set `LVP = ON` before the first flash.
- [ ] **Confirm the MCLR pull-up.** The reset pin is enabled, so without a pull-up the chip may keep resetting.
- [ ] **Check the VUSB3V3 connection.** The design notes flag this pin on the LF part. Check it against the datasheet on both board types, including daughters that don't use USB.
- [ ] **Check the clock startup sequence against the datasheet.** The `OSCTUNE`, `OSCCON` and `ACTCON` sequence in `common/clock.c` comes from memory of Microchip's USB demo code.

## 2. Hardware design

- [ ] **Decide on downstream pull-ups.** Pull-ups are on the motherboard only, so SDA and SCL float on any board behind an open gate. That can cause false bus activity, and a PCA9517 needs pull-ups on both sides anyway. Each board probably needs weak pull-ups on its downstream side, sized so four boards in parallel stay within the I2C spec.

## 3. First tests on real boards

- [ ] **Run the scope test on each board type.** Build with `BRINGUP_TICK_PIN` defined and probe the test pin. See [motherboard/main.c](motherboard/main.c.md) for what the results mean. This is the first check that `clock_init()` really completes, which the simulator can't show.
- [ ] **Confirm the real clock speed.** A 500 Hz square wave means the clock and tick are right.

## 4. Firmware to write

### Motherboard

- [ ] **Chain enumerator.** Runs RESET_CHAIN, then HELLO, reply, decision and ACCEPT for each board.
  - Stop at position 4, and skip the 100 ms HELLO when the chain is full, so a fifth board just stays unregistered.
  - Treat a NACK from `dp_hello()` as the normal end of the chain.
  - Reject the reserved IDs 0x0000 and 0xFFFF, because `dp_read_reply()` accepts them if the complement bytes match.
  - Wait `GATE_SETTLE_MS` after ACCEPT, then confirm the board with `dp_read_info()`.
- [ ] **Daughter database.** Stores seen boards in EEPROM and makes the ACCEPT or REGENERATE decision.
- [ ] **Polling schedule.** Polls each position every 5 ms from `main.c`.
  - When `dp_read_events()` returns an error, still process the first `*count` events before handling the error. The daughter has already removed them and won't send them again.
- [ ] **Module manager.** Counts NACKs toward `NACK_LIMIT`, sends key releases and detach reports for a board that drops out, then re-enumerates.
- [ ] **USB driver.** The HID reports, report queue and event aggregator, connected to `main.c`.
- [ ] **Unused pins.** The design says to drive unused pins low at boot, which isn't done yet because it needs the final pin list from the schematic.

### Daughter board

- [ ] **Real identity storage.** Replace `identity_stub.c` with EEPROM storage and real random IDs, then delete the stub.
  - `identity_lookup()` and `identity_new_id()` must finish well inside the 2 ms I2C timeout.
  - `identity_save()` shouldn't block. A slow EEPROM write could make a HELLO reply miss the timeout.
- [ ] **Input type detection.** Read the ID resistor at boot, and pass the real type and count to `i2c_slave_init()` in place of `INPUT_TYPE_NONE`.
- [ ] **Input drivers.** Key switches, rotary encoder, slide potentiometer and cockpit switches.
  - Encoder: a corrupted I2C ACK can deliver an event twice. That's mostly harmless for keys, but a repeated count adds an extra encoder step, so decide how to handle it.

## 5. Tests once the I2C traffic is running

The I2C drivers have only been compiled so far, and have not run on hardware or in a simulator.

- [ ] **Test the full handshake** with a logic analyzer, including REGENERATE and re-enumeration.
- [ ] **Test the event retry.** Disconnect a board in the middle of a read and confirm that no event is lost.
- [ ] **Test bus recovery.** Force a stuck bus and confirm the motherboard frees it.
- [ ] **Check the bus load.** Polling four idle boards takes about 1.6 ms of each 5 ms cycle. Four full event queues take about 13 ms, so polling briefly falls behind. Confirm that's acceptable in practice.

## 6. Open design questions

- [ ] **I2C bus speed.** The two summary documents disagree on 100 kHz and 400 kHz. The code uses 100 kHz for now, and could try 400 kHz once the bus is proven.
- [ ] **Cockpit switch count.** How many switches are on each board, and how many are 3-position? The driver provides 8 snapshot registers.
- [ ] **Encoder resolution.** The code assumes 4 counts per detent. Check this against the actual part.

## 7. Documents to update

- [ ] **Firmware Driver Summary, key 5 example.** It shows the daughter replying `01 05 01`. The bus now carries `01 05 01 FF`, because the read ends on one extra byte.
- [ ] **Firmware Driver Summary, chain length.** It assumes up to 8 positions. The code uses 4.
- [ ] **Keystroke diagram, bus recovery.** It shows recovery on "NACK or timeout". The code recovers on a timeout or collision, not on a NACK, because HELLO gets a NACK at the end of the chain every 100 ms.
- [ ] **`DriverDocs` pages.** These don't update automatically, so edit them whenever the drivers change.

## 8. Development setup

- [ ] **Set up the MPLAB projects.** Use one project per board, with the source files and include paths listed in the [README](README.md).
- [ ] **Fix the VS Code include paths (optional).** The `cannot open source file "stdint.h"` errors come from IntelliSense not knowing where the XC8 headers are. They don't affect the build.
