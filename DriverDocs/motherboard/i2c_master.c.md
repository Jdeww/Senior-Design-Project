# i2c_master.c

Source: `Drivers/motherboard/i2c_master.c`
Interface: [i2c_master.h](i2c_master.h.md)

## What it does

Runs the motherboard's side of the I2C bus using the PIC's MSSP hardware. It sends and receives bytes at 100 kHz, puts a time limit on every hardware wait so a bad pogo contact can't freeze the firmware, and frees the bus if a daughter is stuck holding it.

## Inputs and outputs

The function-level inputs and outputs are listed in [i2c_master.h](i2c_master.h.md). This file also interacts directly with the hardware:

| Direction | Item | Use |
|---|---|---|
| Input | `g_ms` | Time base for every timeout |
| Input | `PIR1.SSPIF` | Set by the hardware when an operation finishes |
| Input | `PIR2.BCLIF` | Set by the hardware on a bus collision |
| Input | `SSP1CON2.ACKSTAT` | Whether the daughter acknowledged the last byte |
| Input | `SSP1BUF` | The received byte |
| Input | `RB0`, `RB1` | Actual line levels, read during recovery |
| Output | `SSP1BUF` | The byte to send |
| Output | `SSP1CON2` bits | Start each bus operation |
| Output | RB0 (SDA), RB1 (SCL) | Bus lines, driven by the MSSP or directly during recovery |

## How it works

### Setup: `i2c_init()`

1. Turns the MSSP off.
2. Sets RB0 and RB1 to digital inputs, so the MSSP can control them.
3. Sets the clock divider: `SSP1ADD = 48 MHz / (4 x 100 kHz) - 1 = 119`.
4. Clears the done and collision flags.
5. Turns the MSSP on in I2C master mode (`SSP1CON1 = 0x28`).

### Waiting with a timeout: `wait_done()`

Every bus operation follows the same pattern: clear `SSPIF`, start the operation, then call `wait_done()`. It loops until one of these happens:

| Condition | Result |
|---|---|
| `SSPIF` is set | `I2C_OK`, or `I2C_COLLISION` if `BCLIF` is also set |
| `BCLIF` is set | `I2C_COLLISION` |
| More than `I2C_TIMEOUT_MS` (2) has passed | `I2C_TIMEOUT` |

The check uses `>` rather than `>=` because the next tick could arrive just after the wait begins. As a result, the real limit is between 2 and 3 ms. That also sets how long a daughter can stretch the clock during one operation.

Only the low byte of `g_ms` is used. A single byte can't change partway through being read, and subtraction on `uint8_t` still gives the right elapsed time when the counter wraps.

### Bus operations

| Function | Hardware action |
|---|---|
| `send_start()` | Sets `SEN` |
| `send_restart()` | Sets `RSEN` |
| `send_byte()` (private) | Writes `SSP1BUF`, waits, then returns `I2C_NACK` if `ACKSTAT` is set |
| `i2c_stop()` | Sets `PEN` |
| `i2c_receive()` | Sets `RCEN`, waits, then reads `SSP1BUF` |
| `i2c_acknowledge()` | Sets `ACKDT` (0 = ACK, 1 = NACK), then `ACKEN` |

The address byte is the 7-bit address shifted left, with the lowest bit set for a read. For example, position 2 (0x32) becomes 0x64 for a write and 0x65 for a read.

### Transactions

| Function | Bus sequence |
|---|---|
| `i2c_write()` | START, address + W, data bytes, STOP |
| `i2c_read()` | START, address + R, data bytes, STOP |
| `i2c_begin_read_reg()` | START, address + W, register, RESTART, address + R |
| `i2c_read_reg()` | `i2c_begin_read_reg()`, data bytes, STOP |

During a read, the private `read_bytes()` helper sends ACK after every byte except the last, which gets a NACK to tell the daughter the read is over.

Each transaction stops at the first failure and passes the status to `i2c_abort()`.

### Handling failures: `i2c_abort()`

- **NACK:** sends STOP and returns `I2C_NACK`. A NACK means SDA was free, so the bus isn't stuck. If that STOP fails, it falls through to recovery.
- **Anything else:** runs `i2c_bus_recover()`. It returns the original error if recovery works, or `I2C_BUS_STUCK` if not.

Recovery doesn't run on a NACK because HELLO gets one at the end of the chain every 100 ms during normal operation. The keystroke diagram shows recovery on "NACK or timeout", so the code differs from the diagram here.

### Freeing a stuck bus: `i2c_bus_recover()`

A contact glitch can leave a daughter partway through sending a byte and holding SDA low. Recovery works like this:

1. Turns the MSSP off and takes direct control of the pins.
2. Sets both output latches to 0, so each line is either driven low (`TRIS = 0`) or released to the pull-up (`TRIS = 1`). The pins never drive high, which keeps the bus open-drain.
3. Pulses SCL up to 9 times at about 100 kHz, stopping early once SDA goes high. Each pulse lets the daughter move on to its next bit.
4. Always sends a manual STOP: SDA goes low, then SCL rises, then SDA rises. This resets a daughter that was midway through sending a 1, which looks free but isn't finished.
5. Checks that both lines are high, calls `i2c_init()`, and returns `I2C_OK` or `I2C_BUS_STUCK`.

Each time SCL is released, `release_scl()` waits for it to actually rise, with the same time limit. If a daughter keeps SCL low past that limit, recovery gives up and returns `I2C_BUS_STUCK`.
