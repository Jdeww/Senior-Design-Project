# usb_descriptors.c

Source: `Drivers/motherboard/usb_descriptors.c`

## What it does

Holds the descriptors the PC reads during USB enumeration. They describe the motherboard as a vendor-defined HID device, so the PC uses its built-in HID driver and the PC app reads the raw reports. The [MLA stack](mla_usb.md) sends them; this file only provides the data, under the names the stack looks for.

## Inputs and outputs

| Name | Contents |
|---|---|
| `device_dsc` | USB 2.0, VID 0x04D8, PID 0x003F, version 1.00, one configuration |
| `configDescriptor1`, `USB_CD_Ptr` | One configuration with one HID interface and two endpoints |
| `hid_rpt01` | The HID report descriptor |
| `USB_SD_Ptr` | Strings: language (English), manufacturer, product |

## How it works

### Configuration

| Item | Value |
|---|---|
| Power | Bus powered, 100 mA, no remote wakeup |
| Interface | HID class, no boot protocol |
| Endpoint 1 IN | Interrupt, 8 bytes, every 1 ms: reports to the PC |
| Endpoint 1 OUT | Interrupt, 8 bytes, every 1 ms: commands from the PC |

The HID descriptor must start 18 bytes into the configuration, because `usb_device_hid.c` finds it at that fixed offset. Keep the configuration and interface descriptors first.

### Report descriptor

Usage page 0xFF00 (vendor-defined), with 8 bytes of input and 8 of output and no report ID. The HID layer treats the bytes as opaque. Their meaning is the report format in [protocol.h](../common/protocol.h.md).

Its length, 29 bytes, is also set as `HID_RPT01_SIZE` in [usb_config.h](usb_config.h.md). If the descriptor changes, update both.

### Placeholders to replace

| Item | Current value | What to do |
|---|---|---|
| VID and PID | 0x04D8 / 0x003F, from Microchip's custom HID demo | Fine for bench testing. Request a free PID from Microchip's sublicensing program before anyone else uses the device, and update the PC app to match. |
| Maximum power | 100 mA | Measure the motherboard with a full chain and set it with some margin |
| Manufacturer string | "Macro Pad Team" | Replace with the team or organisation name |
