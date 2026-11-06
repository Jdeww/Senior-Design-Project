# daughter_protocol.h

Source: `Drivers/motherboard/daughter_protocol.h`
Implementation: [daughter_protocol.c](daughter_protocol.c.md)

## What it does

Defines the functions the motherboard uses to talk to daughter boards, along with the data structures they return. Higher-level code such as the chain enumerator can call `dp_hello()` or `dp_read_events()` without handling raw I2C bytes.

## Inputs and outputs

Every function returns an `i2c_status_t`, described in [i2c_master.h](i2c_master.h.md).

### Handshake functions

| Function | Inputs | Output | Sent to |
|---|---|---|---|
| `dp_reset_chain()` | None | Status | All boards (0x00) |
| `dp_hello(mb_id)` | Motherboard ID | Status | Discovery (0x2F) |
| `dp_read_reply(reply)` | Where to store the reply | Status; `*reply` is set on success | Discovery (0x2F) |
| `dp_accept(id, pos)` | Board's ID; position 1..4 | Status | Discovery (0x2F) |
| `dp_regenerate(id)` | Board's ID | Status | Discovery (0x2F) |

### Registered board functions

| Function | Inputs | Output |
|---|---|---|
| `dp_read_info(pos, info)` | Position; where to store the info | Status; `*info` is set on success |
| `dp_read_snapshot(pos, input, value)` | Position; input number; where to store the value | Status; `*value` is set on success |
| `dp_read_events(pos, events, max, count)` | Position; event array; array size; where to store the number read | Status; `events[]` and `*count` are set |

Functions that take a position return `I2C_BAD_DATA` without touching the bus if it is outside 1..`CHAIN_MAX_POSITIONS`.

### Data structures

| Structure | Fields |
|---|---|
| `dp_reply_t` | `flag` (`REPLY_FLAG_SEEN` or `REPLY_FLAG_NEW`), `id`, `type` |
| `dp_info_t` | `type`, `count`, `fw_version`, `id` |
| `dp_event_t` | `index` (which input), `value` (its new value) |

## How it works

These functions are meant to be called by the chain enumerator and main loop, which aren't written yet. One board's registration would run like this:

```text
dp_hello(mb_id)        I2C_NACK here means the end of the chain
dp_read_reply(&reply)
   decide using the daughter database
   clash:   dp_regenerate(reply.id), then dp_read_reply() again
   allowed: dp_accept(reply.id, next_position)
wait GATE_SETTLE_MS
dp_read_info(next_position, &info)   confirms the board moved to its new address
```

### Status results that need care

| Call | Result | What the caller should do |
|---|---|---|
| `dp_hello()` | `I2C_NACK` | Treat as the normal end of the chain |
| `dp_read_reply()` | `I2C_BAD_DATA` | Restart enumeration, since all the retries failed |
| Reads from a position | `I2C_NACK` | Count toward `NACK_LIMIT` (2 in a row triggers re-enumeration) |
| `dp_reset_chain()` | `I2C_NACK` | Expect this when no boards are connected |
| `dp_read_events()` | Any error | Process the first `*count` events anyway, because the daughter has already removed them |
