# i2c_slave.c

Source: `Drivers/daughterboard/i2c_slave.c`
Interface: [i2c_slave.h](i2c_slave.h.md)

## What it does

Runs the daughter board's side of the I2C bus. It does four jobs:

1. Answers the motherboard using the MSSP in slave mode.
2. Runs the registration handshake, including changing the board's address.
3. Controls the chain gate that connects the next board.
4. Serves the register map and the 16-entry event queue.

## Inputs and outputs

The function-level inputs and outputs are listed in [i2c_slave.h](i2c_slave.h.md). This file also has these connections:

| Direction | Item | Use |
|---|---|---|
| Input | I2C messages | RESET_CHAIN, HELLO, ACCEPT, REGENERATE and register selections |
| Input | `identity_lookup()`, `identity_new_id()` | The ID to put in the reply |
| Input | `i2c_slave_push_event()`, `i2c_slave_set_snapshot()` | Input data from the main loop |
| Output | I2C reads | The 8-byte reply, register values and events |
| Output | `GATE_LAT` | Opens and closes the chain gate |
| Output | `SSP1ADD` | The address this board answers on |
| Output | `identity_save()` | Stores a new motherboard and ID pair |

## How it works

### Setup: `i2c_slave_init()`

Stores the input type and count, then:

- Drives the gate pin low, so the gate stays open.
- Sets RB0 and RB1 to digital inputs.
- Sets the address to 0x2F (`SSP1ADD` holds the address shifted left one bit).
- Turns on general call (`GCEN`), so the board hears RESET_CHAIN.
- Turns on clock stretching (`SEN`), so the clock is held after every byte until the firmware releases it.
- Turns on the STOP interrupt (`PCIE`), so the firmware knows when a message has ended.
- Enables the MSSP in 7-bit slave mode and makes its interrupt high priority.

### Handshake states

| State | Address | Gate | Event | Action |
|---|---|---|---|---|
| `ST_WAITING` | 0x2F | Open | HELLO | Schedule the ID lookup, go to `ST_REPLYING` |
| `ST_REPLYING` | 0x2F | Open | ACCEPT with my ID and a position of 1..4 | Move to the position address, close the gate, go to `ST_ACTIVE` |
| `ST_REPLYING` | 0x2F | Open | REGENERATE with my ID | Schedule a new random ID |
| `ST_REPLYING` | 0x2F | Open | HELLO | Schedule the lookup again |
| `ST_ACTIVE` | 0x30 + position | Closed | Register read | Return register data |
| Any | Any | Any | RESET_CHAIN | Open the gate, return to 0x2F, go to `ST_WAITING` |

RESET_CHAIN keeps the board's ID and any queued events. ACCEPT or REGENERATE naming a different ID, or arriving before the reply is ready, is ignored.

### The interrupt: `i2c_slave_isr()`

Each MSSP event runs the interrupt once. It works out what happened in this order:

| Check | Meaning | Action |
|---|---|---|
| `SSPOV` or `WCOL` set | Hardware overflow or write collision | Clear the errors and release the clock |
| `P` set | STOP | `handle_message()` |
| `D_nA` = 0, `R_nW` = 1 | Address match, read | `begin_read()` |
| `D_nA` = 0, `R_nW` = 0 | Address match, write | Clear the buffer, note whether it is a general call |
| `D_nA` = 1, `R_nW` = 1 | The master finished a byte of a read | Load the next byte if it sent ACK; send nothing if it sent NACK |
| `D_nA` = 1, `BF` = 1 | Data byte received | `receive_byte()` |
| Otherwise | Master NACK, seen with `R_nW` already cleared | Release the clock |

The last case exists because `R_nW` is only valid up to a NACK, so a NACK can show up with `R_nW` = 0 and no byte received.

### Why messages take effect at the STOP

`handle_message()` acts on RESET_CHAIN, HELLO, ACCEPT and REGENERATE only once the STOP arrives. If a daughter moved its gate as soon as the bytes arrived, boards further down the chain would be cut off before seeing the STOP, and could be left mid-transaction.

The STOP interrupt fires for every STOP on the bus, including other boards' traffic. `handle_message()` returns immediately when nothing was received.

The exception is selecting a register. `receive_byte()` applies it right away, because the motherboard follows it with a RESTART and there is no STOP first.

### Preparing the reply: `i2c_slave_task()`

The EEPROM lookup is too slow for an interrupt, so the interrupt only sets `s_job`, and the main loop does the work:

1. With the MSSP interrupt off, copies and clears the pending job and save flag.
2. If a new pair needs saving, calls `identity_save()`. This runs first so a HELLO straight after a reset finds the pair.
3. Gets the ID with `identity_lookup()` for HELLO, or `identity_new_id()` for REGENERATE.
4. With the interrupt off again, checks that no new job has arrived and the state is still `ST_REPLYING`. If so, calls `publish_reply()`. If not, the answer is out of date and is thrown away.

`publish_reply()` builds the 8 bytes, with the last four as the inverse of the first four, and sets `s_reply_ready`.

### Holding the clock until the reply is ready

The motherboard may read the reply before the lookup has finished. In that case `begin_read()` sets `s_clock_held` and returns without releasing the clock, so SCL stays low and the motherboard waits. `publish_reply()` then loads the first byte and releases the clock.

This wait must stay under the motherboard's timeout of about 2 ms, so the main loop has to keep running quickly.

### What a read returns

The result depends on the state:

| State | Read returns |
|---|---|
| `ST_WAITING` | 0xFF, which fails the motherboard's complement check |
| `ST_REPLYING` | The 8-byte reply, restarting from its first byte on every read, so the motherboard can retry |
| `ST_ACTIVE` | Register data or the event stream |

Register map in `ST_ACTIVE`:

| Register | Returns |
|---|---|
| 0x00 | Input type |
| 0x01 | Input count |
| 0x02 | Firmware version |
| 0x03, 0x04 | ID high, ID low |
| 0x05 | Event count; also starts the event stream if the read begins here |
| 0x06 | Chain position |
| 0x20..0x27 | Snapshot values for inputs 0..7 |
| Anything else | 0xFF |

### Event queue

The queue holds 16 `[index][value]` entries and is safe to share between the main loop and interrupt without turning interrupts off:

- Only the main loop writes `s_fifo_head`, and it updates the head only after writing the data.
- Only the interrupt writes `s_fifo_tail`.
- Both are free-running 8-bit counters, so `head - tail` is always the count, even after they wrap. `counter & 15` gives the slot.

A read that begins at `REG_EVENTS` sends the count first, then each index and value. `next_stream_byte()` runs only after the motherboard ACKs the previous byte, and it removes an event once its value byte has been ACKed. If a read fails at any point, the unconfirmed event stays queued and is sent again on the next poll.

The motherboard ends the read by NACKing one extra byte after the last value. That byte is 0xFF, or the next event's index if more are queued. Either way, the NACK means nothing further is removed.

If the motherboard's ACK after a value byte gets corrupted on the bus, the event stays queued and is sent twice. See [daughter_protocol.c](../motherboard/daughter_protocol.c.md) for what that means for the caller.

### Message validation

`receive_byte()` stores up to 4 bytes, the length of the longest message. A longer message ends up with a length of 5, which matches no message, so it is ignored.
