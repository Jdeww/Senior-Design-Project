# Driver Docs

One page for each `.c` and `.h` file in `Drivers/`, in the same folder layout. Each page covers what the file does, its inputs and outputs, and how it works.

**Status:** both firmwares build for the PIC18LF25K50 with XC8 v4.00. Nothing has run on hardware yet. In the simulator, the tick and main loops ran with the clock wait skipped, but the I2C drivers have not run at all. Each page's Testing section, where it has one, gives the details. Remaining work is in the [to-do list](TODO.md).

## Files

| Doc | Runs on | Summary |
|---|---|---|
| [common/protocol.h](common/protocol.h.md) | Both boards | Addresses, message bytes, registers and timings that both boards must agree on |
| [common/clock.h](common/clock.h.md) | Both boards | Public interface of oscillator startup |
| [common/clock.c](common/clock.c.md) | Both boards | Sets the configuration bits and starts the oscillator |
| [common/system_tick.h](common/system_tick.h.md) | Both boards | Public interface of the 1 ms time base |
| [common/system_tick.c](common/system_tick.c.md) | Both boards | Runs Timer2 and counts milliseconds in `g_ms` |
| [motherboard/board.h](motherboard/board.h.md) | Motherboard | Clock speed and I2C pins |
| [motherboard/main.c](motherboard/main.c.md) | Motherboard | Startup, interrupt handler and main loop |
| [motherboard/i2c_master.h](motherboard/i2c_master.h.md) | Motherboard | Public interface of the I2C master, including status codes |
| [motherboard/i2c_master.c](motherboard/i2c_master.c.md) | Motherboard | Drives the bus using the MSSP hardware and recovers a stuck bus |
| [motherboard/daughter_protocol.h](motherboard/daughter_protocol.h.md) | Motherboard | Public interface for talking to daughter boards |
| [motherboard/daughter_protocol.c](motherboard/daughter_protocol.c.md) | Motherboard | Builds and checks the protocol's messages and register reads |
| [daughterboard/board.h](daughterboard/board.h.md) | Daughter | Clock speed, I2C pins and the chain gate pin |
| [daughterboard/main.c](daughterboard/main.c.md) | Daughter | Startup, interrupt handlers and main loop |
| [daughterboard/identity.h](daughterboard/identity.h.md) | Daughter | Functions that identity storage must provide |
| [daughterboard/identity_stub.c](daughterboard/identity_stub.c.md) | Daughter | Temporary stand-in for identity storage, with no EEPROM |
| [daughterboard/i2c_slave.h](daughterboard/i2c_slave.h.md) | Daughter | Public interface of the I2C slave |
| [daughterboard/i2c_slave.c](daughterboard/i2c_slave.c.md) | Daughter | Answers the bus, runs the handshake, controls the gate and queues events |

## How the files fit together

```text
MOTHERBOARD                                 DAUGHTER BOARD

main.c, then enumerator (not written)       main.c, then input drivers (not written)
        |                                           |
daughter_protocol.c                         i2c_slave.c  <-->  identity_stub.c (temporary)
        |                                           |
i2c_master.c                                MSSP hardware (slave mode)
        |                                           |
MSSP hardware (master mode)  ====== I2C bus ========+
```

`common/protocol.h` is included on both sides, so the two firmwares use the same values. `common/system_tick` runs on both boards too, and the motherboard's I2C timeouts depend on it.

## Terms

| Term | Meaning |
|---|---|
| MSSP | The PIC's built-in serial hardware, used here in I2C mode |
| Position | A board's place in the chain, 1 to 4, which sets its address (`0x30 + position`) |
| Gate closed | The switch conducts, so the next board is connected to the bus |
| Gate open | The switch is off, so the next board cannot hear the bus |
| Clock stretching | A daughter holds SCL low to make the motherboard wait |
| ACK / NACK | The receiver's response after each byte: ACK means received, NACK means no answer or stop sending |

## Building

Each firmware is its own build, with its own board folder on the include path.

| Firmware | Source files | Include paths |
|---|---|---|
| Motherboard | `motherboard/main.c`, `i2c_master.c`, `daughter_protocol.c`, `common/clock.c`, `common/system_tick.c` | `Drivers/motherboard`, `Drivers/common` |
| Daughter | `daughterboard/main.c`, `i2c_slave.c`, `identity_stub.c`, `common/clock.c`, `common/system_tick.c` | `Drivers/daughterboard`, `Drivers/common` |

To build the scope test, also define `BRINGUP_TICK_PIN`.

## Check before flashing

- **Placeholder pins:** the gate pin (RB2, driven in every daughter build) and the test pin (RA1, driven only in test builds) are not from the design documents.
- **Programmer:** `LVP` is off, so programming needs a programmer that supplies high voltage on MCLR.
- **MCLR:** the reset pin is enabled, so the board needs a pull-up on it.
- **VUSB3V3:** the design notes call for checking how this pin should be connected on the LF part, including on daughters that don't use USB.

## Not written yet

- **Chain enumerator and daughter database:** run the handshake on the motherboard using `daughter_protocol`.
- **USB driver:** sends reports to the PC.
- **Real identity storage:** replaces `identity_stub.c` with EEPROM storage and real random IDs.
- **Input detection and input drivers:** read the ID resistor and the keys, encoder, slider or switches on each daughter.
