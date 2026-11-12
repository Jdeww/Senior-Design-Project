# usb_config.h

Source: `Drivers/motherboard/usb_config.h`

## What it does

Holds the compile-time settings for the [MLA USB stack](mla_usb.md). The stack includes this file by name with no folder, so it has to sit in `Drivers/motherboard`, which is on the motherboard's include path.

## Inputs and outputs

This file has no functions. Its main settings:

| Setting | Value | Why |
|---|---|---|
| `USB_INTERRUPT` | Defined | The stack runs in the high-priority interrupt. The main loop can block for tens of milliseconds, which is too long for the PC's control requests to wait. |
| `USB_PING_PONG_MODE` | Full ping-pong | The hardware can take the next packet while the firmware handles the last |
| `USB_SPEED_OPTION` | Full speed | 12 Mbit/s, using the 48 MHz clock set up by [clock.c](../common/clock.c.md) |
| `USB_PULLUP_OPTION`, `USB_TRANSCEIVER_OPTION` | Internal | No external parts needed on D+ and D- |
| `USB_EP0_BUFF_SIZE` | 8 | Control endpoint packet size |
| `USB_MAX_EP_NUMBER` | 1 | Only endpoints 0 and 1 are used |
| `USB_ENABLE_STATUS_STAGE_TIMEOUTS` | 45 ms | Lets a control transfer finish within USB 2.0 timing even if the firmware is slow |
| `USER_USB_CALLBACK_EVENT_HANDLER` | `usb_app_event_handler` | The function in [usb_app.c](usb_app.c.md) the stack calls for each event |
| `HID_EP`, `HID_INTF_ID` | 1, 0 | The HID interface and its endpoint |
| `HID_INT_IN_EP_SIZE`, `HID_INT_OUT_EP_SIZE` | `USB_REPORT_LEN` (8) | Report size, from `protocol.h` |
| `HID_RPT01_SIZE` | 29 | Length of the report descriptor in [usb_descriptors.c](usb_descriptors.c.md). Change both together. |
