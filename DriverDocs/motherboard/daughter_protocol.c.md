# daughter_protocol.c

Source: `Drivers/motherboard/daughter_protocol.c`
Interface: [daughter_protocol.h](daughter_protocol.h.md)

## What it does

Turns protocol actions into I2C traffic. It builds the handshake messages, checks replies for corruption, and unpacks register data into structures. It doesn't touch hardware directly; everything goes through [i2c_master.c](i2c_master.c.md).

## Inputs and outputs

The function-level inputs and outputs are listed in [daughter_protocol.h](daughter_protocol.h.md). Internally:

| Direction | Item | Use |
|---|---|---|
| Input | `protocol.h` | Message bytes, addresses, registers and limits |
| Input | Bytes read through `i2c_master` | Replies, registers and events from daughters |
| Output | Calls to `i2c_master` | All bus traffic |

## How it works

### Sending messages

`dp_reset_chain()`, `dp_hello()`, `dp_accept()` and `dp_regenerate()` each fill a small array with the message bytes and pass it to `i2c_write()`. IDs are sent high byte first.

For example, `dp_accept(0x3AC7, 2)` writes `A1 3A C7 02` to address 0x2F.

### Reading and checking a reply: `dp_read_reply()`

1. Reads 8 bytes from 0x2F.
2. Calls `reply_is_valid()`, which checks that:
   - each of the first four bytes XORed with its complement equals 0xFF
   - the flag is `REPLY_FLAG_SEEN` or `REPLY_FLAG_NEW`
3. If the reply is valid, copies the flag, ID and type into `*reply` and returns `I2C_OK`.
4. If it isn't valid, reads again, up to `REPLY_RETRIES` (3) more times, for 4 reads in total. It then returns `I2C_BAD_DATA`.

A bus error such as a NACK or timeout is returned immediately, without retrying.

This check does not reject the reserved IDs 0x0000 and 0xFFFF, or check that the input type is valid. Any such check is left to the calling code.

### Reading board information: `dp_read_info()`

Reads 5 bytes in one transaction, starting at `REG_INPUT_TYPE`, and relies on the daughter's register pointer moving forward after each byte:

| Byte | Register | Stored in |
|---|---|---|
| 0 | `REG_INPUT_TYPE` | `info->type` |
| 1 | `REG_INPUT_COUNT` | `info->count` |
| 2 | `REG_FW_VERSION` | `info->fw_version` |
| 3, 4 | `REG_ID_HI`, `REG_ID_LO` | `info->id` |

### Reading one input's current value: `dp_read_snapshot()`

Reads one byte from register `REG_SNAPSHOT_BASE + input`. The input number isn't range-checked here. The daughter returns 0xFF for a register it doesn't have.

### Reading events: `dp_read_events()`

The daughter sends a count followed by that many `[index][value]` pairs. The number of bytes isn't known until the count arrives, so this function uses the separate read steps from `i2c_master`:

1. Starts a read at `REG_EVENTS`.
2. Receives the count byte.
3. Decides how many events to read:
   - count above `EVENT_FIFO_DEPTH` (16): 0, because the count must be corrupt
   - otherwise: the smaller of the count and `max`
4. Sends ACK if any events are wanted, or NACK if none are. The usual case of no events costs only this one byte.
5. Reads each index and value, sending ACK after every byte. An event counts as received once the ACK after its value byte has gone out.
6. If any events were wanted, reads one extra byte, ignores it and sends NACK to end the read.
7. Sends STOP and sets `*count`.

The extra byte exists because the ACK after a value byte is what tells the daughter to remove that event. If the last value byte got the NACK instead, the daughter couldn't tell a normal end of read from a released or disconnected line.

For example, one event (key 5 pressed) looks like this, with the motherboard's response in brackets:

```text
01 [ACK]  05 [ACK]  01 [ACK]  FF [NACK]  STOP
```

| Situation | Result |
|---|---|
| Normal read | `I2C_OK`; `*count` is the number read |
| No events | `I2C_OK`; `*count` is 0 |
| More events than `max` | `I2C_OK`; the extras stay on the daughter for the next poll |
| Corrupt count | `I2C_BAD_DATA`; `*count` is 0 and no events are taken |
| Bus error | The error, after cleanup; `*count` is the number fully received before the failure |

After a bus error, the caller must still process the first `*count` events, because the daughter has already removed them. An event that was not confirmed stays on the daughter and is sent again on the next poll.

Events carry no integrity check, unlike the HELLO reply. If the motherboard's ACK after a value byte gets corrupted on the bus, the daughter sends that event again, so it arrives twice. A repeated key event does little harm, but a repeated encoder count would add an extra step.
