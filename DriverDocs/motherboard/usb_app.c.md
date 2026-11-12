# usb_app.c

Source: `Drivers/motherboard/usb_app.c`
Interface: [usb_app.h](usb_app.h.md)

## What it does

Connects the [MLA USB stack](mla_usb.md) to the rest of the motherboard. It sends the [report queue](report_queue.h.md) to the PC on endpoint 1 IN, and carries out the PC's commands from endpoint 1 OUT.

## Inputs and outputs

The function-level inputs and outputs are listed in [usb_app.h](usb_app.h.md). This file also interacts with:

| Direction | Item | Use |
|---|---|---|
| Input | `report_queue_pop()` | The next report for the PC |
| Input | Endpoint 1 OUT | Commands from the PC |
| Output | Endpoint 1 IN | 8-byte reports to the PC |
| Output | `module_manager_list_modules()` | Runs the list modules command |
| Output | `report_push_fw_version()` | Answers the firmware version command |
| Input | `usb_app_event_handler()` | Called by the stack for USB events, in the interrupt |

## How it works

### Two contexts

| Code | Runs in | Job |
|---|---|---|
| `usb_app_event_handler()` | High-priority interrupt, from `USBDeviceTasks()` | Opens endpoint 1 when the PC configures the device, and answers HID requests on endpoint 0 |
| `usb_app_task()` | Main loop | Moves reports and commands |

The main loop masks the USB interrupt for the few cycles it takes to hand a buffer to the stack, so the two never change the endpoint state at the same time.

### Sending reports

On each pass, if the last report has gone, `usb_app_task()` pops the next one from the queue and hands it to the stack. The PC polls endpoint 1 IN once every 1 ms, so at most one report goes per frame.

Until the PC has configured the device, or while the bus is suspended, nothing is popped. Reports wait in the queue, and once it fills, events wait on the daughter boards.

### Commands

When an OUT packet arrives, byte 0 is the command:

| Command | Action |
|---|---|
| `0x10` list modules | `module_manager_list_modules()`. The module manager then sends a chain reset, every attach report, the slider and switch states, and chain complete. |
| `0x11` firmware version | Queues `[06][FW_VERSION]` |
| Anything else | Ignored |

If the version report doesn't fit in the queue, the OUT buffer isn't handed back. The hardware then NAKs the PC's next command until there's room, so no command is lost.

### Endpoint buffers

The USB module can only reach USB RAM (0x400 to 0x7FF). The two 8-byte buffers are placed at fixed addresses straight after the stack's endpoint 0 buffers, at 0x430 and 0x438. A build-time check stops the build if they would run past USB RAM.

### Known limits

- **A bus reset drops the report being sent.** Reports still in the queue are kept.
- **No low-power suspend.** The USB specification limits a suspended device to 2.5 mA, and this firmware keeps running normally. See the [to-do list](../TODO.md).

## Testing

Built with XC8 v3.10 for the PIC18LF25K50, with no warnings from this file. The map file confirms the buffers at 0x430 and 0x438.

**Not tested:** it hasn't run, in the simulator or on hardware. The simulator doesn't model the USB module.
