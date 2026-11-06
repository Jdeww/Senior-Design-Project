# report_queue.h

Source: `Drivers/motherboard/report_queue.h`
Implementation: [report_queue.c](report_queue.c.md)

## What it does

Defines the queue of reports waiting to go to the PC. The PC only collects a report when it polls, once every 1 ms, so bursts such as the attach reports during enumeration wait here. The USB driver, which isn't written yet, will drain it.

## Inputs and outputs

| Function | Inputs | Output |
|---|---|---|
| `report_queue_clear()` | None | None. Empties the queue. |
| `report_queue_free()` | None | Number of free slots, 0 to 40 |
| `report_push_input(position, index, value)` | Board position; input number; its value | `false` if the queue is full |
| `report_push_attach(position, id, type, input_count)` | Board position, ID, input type and input count | `false` if the queue is full |
| `report_push_detach(position, id)` | Board position and ID | `false` if the queue is full |
| `report_push_chain_reset()` | None | `false` if the queue is full |
| `report_push_chain_complete(board_count)` | Number of boards | `false` if the queue is full |
| `report_queue_pop(out)` | A buffer of `USB_REPORT_LEN` (8) bytes | `false` if the queue is empty; otherwise the oldest report is in `out` |

`REPORT_QUEUE_DEPTH` is 40.

## How it works

Call everything from the main loop. When a push returns `false`, nothing is added.

The USB driver should send reports like this on each main loop pass:

```c
uint8_t report[USB_REPORT_LEN];

if (usb_ready_to_send() && report_queue_pop(report)) {
    usb_send(report);
}
```

`usb_ready_to_send()` and `usb_send()` stand for functions the USB driver will provide.
