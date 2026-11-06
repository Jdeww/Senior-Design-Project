# daughter_db.h

Source: `Drivers/motherboard/daughter_db.h`
Implementation: [daughter_db.c](daughter_db.c.md)

## What it does

Defines the interface to the motherboard's permanent record: its own 16-bit ID, and a table of up to 60 daughter boards it has seen. This lets the motherboard tell a returning board from a new one.

## Inputs and outputs

| Function | Inputs | Output |
|---|---|---|
| `db_init()` | None | `false` if a write failed; the ID still works until the next reset, but may not have been saved |
| `db_mb_id()` | None | This motherboard's ID |
| `db_contains(id)` | A daughter ID | `true` if it's in the table |
| `db_save(id, type, position)` | A daughter ID; its input type; its chain position | `false` if a write failed |

## How it works

Call `db_init()` once at boot, with the tick running and interrupts on, before calling anything else here. All of these must be called from the main loop.

`db_save()` updates the existing entry if the ID is known, and otherwise adds one, replacing the oldest entry when the table is full.

| Call | EEPROM writes | Approximate blocking time |
|---|---|---|
| Known board, nothing changed | None | Under 1 ms |
| Known board, new position | 1 byte | 4 ms |
| New board | 6 bytes | 24 ms |
