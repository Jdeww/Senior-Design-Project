# mla_usb (Microchip USB stack)

Source: `Drivers/motherboard/mla_usb/`

## What it does

Microchip's USB device stack, from Microchip Libraries for Applications (MLA). It handles the USB protocol: answering the PC's enumeration requests on endpoint 0, bus reset, suspend and resume, and moving packets on endpoint 1. The HID class file answers HID requests, such as the PC asking for the report descriptor.

The files are copied unchanged from https://github.com/MicrochipTech/mla_usb at commit `d28cc13`, under the Apache 2.0 license. The folder's `README.md` lists them.

## Inputs and outputs

What the project provides to the stack:

| Provided by | What |
|---|---|
| [usb_config.h](usb_config.h.md) | Compile-time settings |
| [usb_descriptors.c](usb_descriptors.c.md) | Device, configuration, report and string descriptors |
| [usb_app.c](usb_app.c.md) | The event callback, `usb_app_event_handler()` |

What the project calls in the stack:

| Function | Called from |
|---|---|
| `USBDeviceInit()`, `USBDeviceAttach()` | `usb_app_init()` |
| `USBDeviceTasks()` | The high-priority interrupt in [main.c](main.c.md) |
| `HIDTxPacket()`, `HIDRxPacket()`, `HIDTxHandleBusy()`, `HIDRxHandleBusy()` | `usb_app.c` |
| `USBEnableEndpoint()`, `USBCheckHIDRequest()` | `usb_app_event_handler()` |

## How it works

- **Leave the files unchanged.** Every project setting lives in the three files above, so a newer copy of the stack can replace these.
- **Interrupt mode:** `USBDeviceTasks()` runs on every USB interrupt, so the PC gets answers even while the main loop is blocked.
- **Fixed RAM:** the stack places its buffer descriptor table at 0x400 and its endpoint 0 buffers at 0x420 and 0x428, in USB RAM.
- **Compiler messages:** XC8 v3.10 gives warnings and advisories from these files, such as unused functions and functions duplicated because both the interrupt and the main loop call them. They come from Microchip's code and are expected.
