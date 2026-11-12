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
/* module_manager_init() */
(void)db_init();
report_queue_clear();
(void)report_push_chain_reset();
s_needs_enumerating = !chain_enumerate();

/* module_manager_task(), every HELLO_INTERVAL_MS */
if (s_needs_enumerating) {
    s_needs_enumerating = !chain_enumerate();
} else if (chain_append() == CHAIN_ERROR) {
    restart_chain();    /* queues reports, then calls chain_enumerate() */
}
```

This is simplified: in the real code, `chain_enumerate()` is called through `run_enumeration()`, which also clears the held inputs and failure counts. When an earlier pass failed, the retry sends no new chain reset report, because one was already queued.

| Use | Function | Effect on other boards |
|---|---|---|
| Startup, after a board drops out, or after `CHAIN_ERROR` | `chain_enumerate()` | Resets every board and re-registers the chain |
| Checking for a board added at the end | `chain_append()` | None |

After `CHAIN_ERROR`, the existing boards stay in the table, but the caller should rebuild it with `chain_enumerate()`. The module manager does this through `restart_chain()`. That function first queues a release for every held input and a chain reset report for the PC. It queues no detach, because no board has hit the failure limit. See [module_manager.c](module_manager.c.md).

A new board blocks the main loop for about 24 ms while its database entry is written.
