# Driver Docs

One page for each `.c` and `.h` file in `Drivers/`, in the same folder layout. Each page covers what the file does, its inputs and outputs, and how it works.

**Status:** this code compiles and links for the PIC18LF25K50 with XC8 v4.00, but the I2C drivers have not been run on hardware or in a simulator. The link check used temporary stand-ins for the missing parts listed at the bottom of this page. The system tick's period has been measured in the simulator, as described in [its page](common/system_tick.c.md).

## Files

| Doc | Runs on | Summary |
|---|---|---|
| [common/protocol.h](common/protocol.h.md) | Both boards | Addresses, message bytes, registers and timings that both boards must agree on |
| [common/system_tick.h](common/system_tick.h.md) | Both boards | Public interface of the 1 ms time base |
| [common/system_tick.c](common/system_tick.c.md) | Both boards | Runs Timer2 and counts milliseconds in `g_ms` |
| [motherboard/board.h](motherboard/board.h.md) | Motherboard | Clock speed and I2C pins |
| [motherboard/i2c_master.h](motherboard/i2c_master.h.md) | Motherboard | Public interface of the I2C master, including status codes |
| [motherboard/i2c_master.c](motherboard/i2c_master.c.md) | Motherboard | Drives the bus using the MSSP hardware and recovers a stuck bus |
| [motherboard/daughter_protocol.h](motherboard/daughter_protocol.h.md) | Motherboard | Public interface for talking to daughter boards |
| [motherboard/daughter_protocol.c](motherboard/daughter_protocol.c.md) | Motherboard | Builds and checks the protocol's messages and register reads |
| [daughterboard/board.h](daughterboard/board.h.md) | Daughter | Clock speed, I2C pins and the chain gate pin |
| [daughterboard/identity.h](daughterboard/identity.h.md) | Daughter | Functions that identity storage must provide (not implemented yet) |
| [daughterboard/i2c_slave.h](daughterboard/i2c_slave.h.md) | Daughter | Public interface of the I2C slave |
| [daughterboard/i2c_slave.c](daughterboard/i2c_slave.c.md) | Daughter | Answers the bus, runs the handshake, controls the gate and queues events |

## How the files fit together

```text
MOTHERBOARD                                 DAUGHTER BOARD

Chain enumerator, polling (not written)     Main loop, input drivers (not written)
        |                                           |
daughter_protocol.c                         i2c_slave.c  <-->  identity (not written)
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

## Not written yet

The firmware cannot run until these exist:

- **Oscillator setup:** runs each CPU at the speed in its `board.h`. Until then, the tick won't actually be 1 ms.
- **Identity storage:** implements the three functions in `identity.h` on the daughter board.
- **`main` for each board:** sets up clocks and interrupts and calls these drivers.
- **Chain enumerator and daughter database:** run the handshake on the motherboard using `daughter_protocol`.
