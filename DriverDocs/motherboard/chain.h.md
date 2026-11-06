# chain.h

Source: `Drivers/motherboard/chain.h`
Implementation: [chain.c](chain.c.md)

## What it does

Defines the chain enumerator's interface: registering daughter boards in physical order, checking for a board added at the end, and looking up which board is at each position.

## Inputs and outputs

| Function | Inputs | Output |
|---|---|---|
| `chain_enumerate()` | None | `true` if the pass finished; `false` if every try failed, leaving an empty table |
| `chain_append()` | None | `CHAIN_NO_CHANGE`, `CHAIN_ADDED` or `CHAIN_ERROR` |
| `chain_count()` | None | Number of registered boards, 0 to 4 |
| `chain_get(position)` | Position, 1 to `chain_count()` | A pointer to that board's details, or a null pointer if there is none |

`chain_board_t` holds `id`, `type`, `input_count` and `fw_version`. A board's position is its place in the table.

## How it works

Call these from the main loop only, after `db_init()`. The [module manager](module_manager.c.md) makes these calls in the real firmware, in this pattern:

```c
(void)db_init();
needs_enumerating = !chain_enumerate();

/* then every HELLO_INTERVAL_MS */
if (needs_enumerating || (chain_append() == CHAIN_ERROR)) {
    needs_enumerating = !chain_enumerate();
}
```

| Use | Function | Effect on other boards |
|---|---|---|
| Startup, or after a board drops out | `chain_enumerate()` | Resets every board and re-registers the chain |
| Checking for a board added at the end | `chain_append()` | None |

After `CHAIN_ERROR`, the existing boards stay in the table, but the caller should call `chain_enumerate()` to rebuild it.

A new board blocks the main loop for about 24 ms while its database entry is written.
