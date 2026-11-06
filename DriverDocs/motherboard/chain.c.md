# chain.c

Source: `Drivers/motherboard/chain.c`
Interface: [chain.h](chain.h.md)

## What it does

Registers daughter boards one at a time and records who is at each position. Because each board's gate stays open until it's accepted, only one unregistered board can hear the bus at a time, so positions come out in physical order.

This file also makes the ACCEPT or REGENERATE decision for each board.

## Inputs and outputs

The function-level inputs and outputs are listed in [chain.h](chain.h.md). This file also interacts with:

| Direction | Item | Use |
|---|---|---|
| Input and output | [daughter_protocol](daughter_protocol.h.md) | All bus traffic |
| Input and output | [daughter_db](daughter_db.h.md) | The motherboard ID, whether a board is known, and saving each board |
| Input | `g_ms` | The settling wait after a gate moves |

## How it works

### Registering one board: `register_board()`

1. Sends HELLO. A NACK means the end of the chain.
2. Reads the reply.
3. Applies the decision table. If the answer is REGENERATE, it sends that and goes back to step 2, up to 5 times.
4. Sends ACCEPT with the position.
5. Waits for the gate to settle (`GATE_SETTLE_MS`).
6. Reads the board's information from its new address, which confirms it moved. The ID must match.
7. Adds the board to the table and saves it to the database.

A failed database save doesn't stop the board being used. The worst outcome is that it looks new next time, which the decision table handles.

### Decision table

| Check, in order | Decision |
|---|---|
| ID is reserved (0x0000 or 0xFFFF) | REGENERATE |
| ID is already in this chain | REGENERATE: two boards claim one ID |
| Flag is new, and ID is in the database | REGENERATE: the random ID clashed with a known board |
| Flag is new, and ID is not in the database | ACCEPT, add |
| Flag is seen, and ID is in the database | ACCEPT, update |
| Flag is seen, and ID is not in the database | ACCEPT, re-add |

The reserved ID check is an addition to the table in the Firmware Driver Summary.

### A full pass: `chain_enumerate()`

1. Empties the table.
2. Sends RESET_CHAIN. A NACK just means no boards are connected.
3. Waits for the gates to settle.
4. Calls `register_board()` for positions 1, 2 and so on, until the end of the chain or position 4.

If any step fails, the table is emptied and the whole pass is tried again, up to 3 passes.

### Checking for a new board: `chain_append()`

Calls `register_board()` for the next position, with no reset. When the chain is full it returns without sending anything, so a fifth board stays unregistered and is never accepted.

### What this leaves to the module manager

This file only keeps the table. The [module manager](module_manager.c.md) reads it to send attach, chain reset and chain complete reports, and to read slider and cockpit switch positions after a board registers.

## Testing

The real `chain.c`, `daughter_db.c` and `eeprom.c` were run in the MPLAB X simulator against fake daughter boards that replace `daughter_protocol.c`. All 206 checks passed.

| Area | What was checked |
|---|---|
| Handshake | The exact scenario in `identity_handshake_sequence.puml`, including a clash and a REGENERATE |
| Decision table | Every row above |
| Chain limit | A fifth board gets no HELLO and stays unregistered |
| No boards | Finishes with an empty table |
| Failures | Bad replies (restart, then give up), a board that doesn't move, a board that always clashes, a bus fault on HELLO |
| Appending | Nothing new, a new board with no reset, and a failure that keeps the existing table |

**Not tested:** the enumerator talking to real daughter firmware over a real bus. The fake boards replace all of the I2C code, so these tests show the logic is right but not that the I2C drivers work. The test files aren't in the repository.
