# usb_app.h

Source: `Drivers/motherboard/usb_app.h`
Implementation: [usb_app.c](usb_app.c.md)

## What it does

Defines how the rest of the motherboard uses USB: starting it, and moving reports to the PC and commands from it. The USB protocol itself is handled by Microchip's [MLA stack](mla_usb.md).

## Inputs and outputs

| Function | Inputs | Output | Call from |
|---|---|---|---|
| `usb_app_init()` | None | None. Starts the USB module and connects to the bus. | Boot |
| `usb_app_task()` | None | None. Sends one queued report and runs any PC command. | Every main loop pass |
| `usb_app_ready()` | None | `true` once the PC has configured the device and the bus isn't suspended | Main loop |

## How it works

### Connecting it to the firmware

```c
void __interrupt(high_priority) high_isr(void)
{
    USBDeviceTasks();
}

void main(void)
{
    /* clock, tick, I2C, interrupts on */
    usb_app_init();         /* before any long blocking work */
    module_manager_init();

    for (;;) {
        usb_app_task();
        module_manager_task();
    }
}
```

`usb_app_init()` turns on interrupt priorities and high-priority interrupts as a side effect, because the stack's attach function does this.

Call it before `module_manager_init()`. The stack runs in the interrupt, so the PC can enumerate the device while the chain is still being registered.
