# report_queue.h

Source: `Drivers/motherboard/report_queue.h`
Implementation: [report_queue.c](report_queue.c.md)

## What it does

Defines the queue of reports waiting to go to the PC. The PC only collects a report when it polls, once every 1 ms, so bursts such as the attach reports during enumeration wait here. [usb_app.c](usb_app.c.md) drains it.

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
| `report_push_fw_version(version)` | Firmware version | `false` if the queue is full |
| `report_queue_pop(out)` | A buffer of `USB_REPORT_LEN` (8) bytes | `false` if the queue is empty; otherwise the oldest report is in `out` |

`REPORT_QUEUE_DEPTH` is 40.

## How it works

Call everything from the main loop. When a push returns `false`, nothing is added.

`usb_app_task()` sends reports like this on each main loop pass, popping straight into the USB endpoint buffer:

```c
if (!HIDTxHandleBusy(s_in_handle)) {
    if (report_queue_pop((uint8_t *)s_in_buffer)) {
        send_in();
    }
}
```
