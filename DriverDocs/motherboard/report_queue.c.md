# report_queue.c

Source: `Drivers/motherboard/report_queue.c`
Interface: [report_queue.h](report_queue.h.md)

## What it does

Holds up to 40 reports in order, and builds each report's bytes in the format the PC expects.

## Inputs and outputs

The function-level inputs and outputs are listed in [report_queue.h](report_queue.h.md). It has no hardware connections and uses the `RPT_*` values and `USB_REPORT_LEN` from `protocol.h`.

| Report | Bytes |
|---|---|
| Input event | `[01][position][index][value]` |
| Attach | `[02][position][id_hi][id_lo][type][input count]` |
| Detach | `[03][position][id_hi][id_lo]` |
| Chain reset | `[04]` |
| Chain complete | `[05][board count]` |

## How it works

It's a ring buffer with three counters: where the next push goes (`s_head`), where the oldest report is (`s_tail`), and how many are stored (`s_count`). Each index wraps back to 0 at the end of the array.

USB reports are 8 bytes, but no report uses more than 6. Only 6 are stored, which saves 80 bytes of RAM. `report_queue_pop()` fills the last 2 bytes with 0.

## Testing

Tested in the MPLAB X simulator, as part of the suite described in [module_manager.c](module_manager.c.md): filling, refusing when full, wrapping around, popping when empty, and the bytes of every report type.
