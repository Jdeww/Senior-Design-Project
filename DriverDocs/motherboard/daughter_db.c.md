# daughter_db.c

Source: `Drivers/motherboard/daughter_db.c`
Interface: [daughter_db.h](daughter_db.h.md)

## What it does

Stores the motherboard's ID and its daughter board table in EEPROM. It generates the ID the first time the board runs, and survives power cuts and damaged data without losing track of the table.

## Inputs and outputs

The function-level inputs and outputs are listed in [daughter_db.h](daughter_db.h.md). This file also interacts with:

| Direction | Item | Use |
|---|---|---|
| Input and output | [eeprom.c](../common/eeprom.c.md) | All stored data |
| Input | ADC, fixed voltage reference, `TMR2`, `g_ms` | Sources of randomness for the motherboard ID |

## How it works

### EEPROM layout

| Address | Contents |
|---|---|
| 0 | Magic value `0xA5` |
| 1, 2 | Motherboard ID, high byte then low byte |
| 3 | Number of entries in use |
| 4 | Slot for the next new entry; once full, this is the oldest entry |
| 5 onward | 60 entries of `[id_hi][id_lo][input type][last position]` |

A build-time check confirms that this fits in the 256-byte EEPROM.

### Startup: `db_init()`

| What it finds | What it does |
|---|---|
| Wrong magic, or a reserved ID (0x0000, 0xFFFF) | Generates a new ID and starts an empty table |
| Count above 60 | Keeps the ID, empties the table |
| Everything valid | Loads the ID, count and next slot |

The magic is written last, so a power cut during first-time setup just starts setup over.

### Adding an entry: `db_save()`

Writes in this order:

1. The 4 entry bytes
2. The next slot
3. The count

A power cut at any point leaves a usable table:

| Power cut | Result at the next boot |
|---|---|
| During the entry bytes, table not full | The entry is outside the count, so it's ignored and later overwritten |
| During the entry bytes, table full | That one entry is damaged, so the board may look new. The decision table handles this. |
| Between next slot and count | `db_init()` recalculates the next slot from the count |

### Generating the motherboard ID

The chip has no random number generator. `generate_id()` starts from the tick, then takes 16 ADC readings of the internal voltage reference, mixing the low bits of each reading with `TMR2`. It never returns 0x0000 or 0xFFFF.

This is a modest amount of randomness. It only runs once in the board's life, and a repeat only matters if two motherboards share daughter boards.

## Testing

Run in the MPLAB X simulator, as part of the suite described in [chain.c](chain.c.md):

| Check | Result |
|---|---|
| New database has a valid ID, and the ID survives a reset | Pass |
| Add, update in place, entries survive a reset | Pass |
| 61st entry replaces the oldest, and this order survives a reset | Pass |
| Corrupt count empties the table and keeps the ID | Pass |
| Both power-cut cases that can be set up by hand | Pass |
| Blank EEPROM and reserved stored IDs | Pass |

**Not tested:** how random the ID is on real hardware. The simulator can't produce real ADC noise. Check this by reading the ID from a few real boards after first boot.
