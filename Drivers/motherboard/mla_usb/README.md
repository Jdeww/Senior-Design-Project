# mla_usb (vendored)

The USB device stack from Microchip Libraries for Applications (MLA), copied without changes.

- Source: https://github.com/MicrochipTech/mla_usb
- Commit: `d28cc13c6b4a539ed2333348efbde7c6546fbfc6` (29 Nov 2018)
- License: Apache 2.0, see `LICENSE`

Only the files the motherboard needs are here: the device core, the HID class and the PIC18 hardware layer. Don't edit them. Every project-specific setting is in `motherboard/usb_config.h`, `usb_descriptors.c` and `usb_app.c`, so a newer copy of the stack can be dropped in over these files.

| File | Purpose |
|---|---|
| `src/usb_device.c`, `src/usb_device_local.h` | Device core: enumeration on endpoint 0, reset, suspend, endpoint transfers |
| `src/usb_device_hid.c` | HID class requests, such as sending the report descriptor |
| `inc/usb.h` and the other headers | Public interface, chapter 9 definitions, PIC18 register layer |

Building with XC8 v3.10 gives some warnings and advisories from these files, such as unused functions and functions duplicated for the interrupt. They come from Microchip's code and are expected.
