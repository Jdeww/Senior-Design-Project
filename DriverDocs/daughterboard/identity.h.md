# identity.h

Source: `Drivers/daughterboard/identity.h`

## What it does

Lists the three functions the I2C slave needs from the board's identity storage. It only declares them: **there is no `identity.c` yet, so the daughter firmware won't link until they are written.**

Identity storage remembers which motherboards this board has joined and which ID it uses on each one, so it can answer HELLO as "seen" with its old ID or "new" with a fresh one.

## Inputs and outputs

| Function | Inputs | Output |
|---|---|---|
| `identity_lookup(mb_id, flag)` | Motherboard ID; where to store the flag | The ID to use. `*flag` is `REPLY_FLAG_SEEN` with the stored ID, or `REPLY_FLAG_NEW` with a fresh random ID. |
| `identity_new_id()` | None | A fresh random ID, never 0x0000 or 0xFFFF |
| `identity_save(mb_id, id)` | Motherboard ID; this board's ID | None. Remembers the pair so the next HELLO from this motherboard answers "seen". |

## How it works

All three are called from `i2c_slave_task()` in the main loop, never from the interrupt.

| Trigger | Function called |
|---|---|
| HELLO received | `identity_lookup()` |
| REGENERATE received | `identity_new_id()` |
| ACCEPT received with a new ID | `identity_save()` |

### Timing requirements for the implementation

- **`identity_lookup()` and `identity_new_id()` must be fast:** the motherboard may be waiting for the reply with the clock held. They must finish well inside `I2C_TIMEOUT_MS` (2 ms), or the motherboard will time out.
- **`identity_save()` should not block:** EEPROM writes are slow, and while it runs, `i2c_slave_task()` can't prepare a reply. It should queue the write and finish it in the background.
