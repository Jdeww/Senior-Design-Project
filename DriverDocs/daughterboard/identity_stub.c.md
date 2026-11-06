# identity_stub.c

Source: `Drivers/daughterboard/identity_stub.c`
Interface: [identity.h](identity.h.md)

## What it does

A **temporary** stand-in for identity storage, so the daughter firmware can link and be flashed before the real EEPROM version exists. Delete it when that version is written.

## Inputs and outputs

| Function | Inputs | Output |
|---|---|---|
| `identity_new_id()` | None | An ID based on the current tick, never 0x0000 or 0xFFFF |
| `identity_lookup(mb_id, flag)` | Motherboard ID (ignored); where to store the flag | A new ID; `*flag` is always `REPLY_FLAG_NEW` |
| `identity_save(mb_id, id)` | Both ignored | None |

## How it works

- **Generating an ID:** XORs `tick_now()` with 0x5A5A. If the result is a reserved ID, it uses 0x5A5A instead.
- **Looking up an ID:** always reports the board as new.
- **Saving:** does nothing.

### How this differs from the real version

| Behaviour | This stub | Real version |
|---|---|---|
| Remembers motherboards | No | Yes, in EEPROM |
| A returning board answers | New, with a different ID | Seen, with its old ID |
| PC key mappings survive a re-enumeration | No, because the ID changes | Yes |
| ID quality | Predictable, and can repeat between boards | Random, from ADC noise and Timer0 |

If two boards pick the same ID, the motherboard's REGENERATE step gets a new one, by which time the tick has moved on.
