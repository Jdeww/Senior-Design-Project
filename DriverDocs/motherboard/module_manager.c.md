# module_manager.c

Source: `Drivers/motherboard/module_manager.c`
Interface: [module_manager.h](module_manager.h.md)

## What it does

Runs the motherboard's chain schedule. It combines three parts from the design: the polling schedule, the event aggregator (which adds the position to each event and tracks held keys), and the module manager (which handles boards dropping out).

## Inputs and outputs

The function-level inputs and outputs are listed in [module_manager.h](module_manager.h.md). This file also interacts with:

| Direction | Item | Use |
|---|---|---|
| Input | `tick_now()` | Timing of polls and HELLOs |
| Input | [daughter_protocol](daughter_protocol.h.md) | Events and slider or switch states from each board |
| Input and output | [chain](chain.h.md) | Which boards are registered; enumerating and appending |
| Output | [report_queue](report_queue.h.md) | Every report for the PC |

## How it works

### Schedule

| When | Action |
|---|---|
| Every call | Announce any boards the PC doesn't know about yet |
| Every 5 ms | Read events from each announced board |
| Every 100 ms | Look for a new board at the end of the chain, or retry enumeration if the last try failed |

### Announcing boards

A new board's attach report goes out, followed by its current state if it's a slider or cockpit board, so the PC starts in sync.

- **A board waits until all of its reports fit in the queue,** so they stay together.
- **A board isn't polled until it has been announced,** so the PC always gets its attach report before any of its events.
- **At most `SNAPSHOT_MAX` (8) states are read for each board,** whatever input count it reports, so a bad count can't stall the queue.

After a full enumeration, a chain complete report follows the last attach. A board appended later gets only an attach.

### Polling

For each announced board, in position order:

1. Works out how many events the report queue has room for, up to 16.
2. Calls `dp_read_events()` with that limit.
3. Sends every event received as an input event report and updates the held inputs, even if the read then returned an error, because the daughter has already removed those events.
4. Clears the board's failure count if the read succeeded, or adds to it if not.

**When the queue is full,** the limit is 0. The poll still happens, because it's also how a dropped board is noticed, but it takes no events. They wait on the daughter, in order, so a slow PC doesn't lose them. If the daughter's own 16-entry queue then fills, its input driver has to handle that.

### Held inputs

One bit per input tracks what's currently held on each board:

| Input | Tracked as held? |
|---|---|
| Key switches | Yes |
| Encoder button (index 1) | Yes |
| Encoder detent count (index 0) | No, it isn't a pressed state |
| Sliders and cockpit switches | No, they stay where they're set |

### When a board drops out

A board counts as lost after `NACK_LIMIT` (2) failed polls in a row, which takes about 10 ms. The pass finishes first, because unplugging a middle board also cuts off every board behind it. `restart_chain()` then queues, in this order:

1. A release for every held input on **every** board, because positions are about to change
2. A detach for each board at the limit
3. A chain reset

It then runs `chain_enumerate()`. The attach reports and chain complete follow through the normal announcing step.

If the queue doesn't have room for those reports, it's emptied first. The chain reset replaces what the dropped reports described, and slider and switch states are sent again afterward. A build-time check confirms that the queue can always hold them.

A failed `chain_append()` also triggers a restart, with no detach. If enumeration itself fails, it's retried every 100 ms without sending another chain reset.

### Differences from the summary documents

- **Any failed poll counts toward the limit, not only NACKs.** A board unplugged part way through a read can look like a timeout or a corrupt count.
- **A detach is sent for each failed board,** where the design describes the single board that stopped answering.
- **Releases are sent for every board.** The Firmware Driver Summary says to send releases for held keys; this applies that across the chain.

### Known behaviour to be aware of

If a key on a working board is held during a restart, the PC gets a release even though the key is still down, and there's no new press until it's physically released and pressed again.

## Testing

The real `module_manager.c`, `report_queue.c`, `chain.c`, `daughter_db.c` and `eeprom.c` were run in the MPLAB X simulator against fake boards that replace `daughter_protocol.c`. All 249 checks passed.

| Area | What was checked |
|---|---|
| Boot | Report order, including a slider's state |
| Events | The key 5 example from the Firmware Driver Summary |
| Schedule | 76 to 82 polls of 2 boards in 200 ms, and 2 to 3 HELLOs |
| Drop-out | One failure is ignored; two restart the chain, with reports in the right order |
| Middle board unplugged | Detach for every board cut off |
| Held inputs | Only keys and the encoder button are released |
| Full queue | Events wait in order with none lost; a dropped board is still noticed and its releases get out |
| Announcing | Waits for room, no polling before the attach report, and a nonsense input count doesn't stall it |
| Failed reads | Events already received are forwarded; timeouts count toward the limit |
| New boards | Attach only, with no reset; a failed append restarts the chain |
| Enumeration failure | Retries every 100 ms with only one chain reset report |

To confirm that these tests can catch real bugs, they were also run against six copies of this file with a bug added on purpose: wrong failure limit, wrong input type tracked as held, ignoring queue room, no detach, no announcing, and dropping events from a failed read. All six were caught.

**Not tested:** real boards over a real bus, or real USB. The test files aren't in the repository.
