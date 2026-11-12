# module_manager.h

Source: `Drivers/motherboard/module_manager.h`
Implementation: [module_manager.c](module_manager.c.md)

## What it does

Defines the two calls that keep the chain running: polling each board for events, turning them into reports for the PC, looking for new boards, and rebuilding the chain when a board drops out.

## Inputs and outputs

| Function | Inputs | Output |
|---|---|---|
| `module_manager_init()` | None | None. Loads the database and registers the chain. |
| `module_manager_task()` | None | None. Runs whatever work is due. |
| `module_manager_list_modules()` | None | None. Asks for the PC's list modules replay: a chain reset, every attach report and slider and switch state, then chain complete. It runs on a later `module_manager_task()` call. |

Its results reach the rest of the firmware as reports in the [report queue](report_queue.h.md).

## How it works

```c
tick_init();
i2c_init();
/* enable interrupts */

module_manager_init();

for (;;) {
    module_manager_task();
}
```

Call `module_manager_init()` once, with the tick running and interrupts on. Then call `module_manager_task()` on every main loop pass. It does its own timing, so it's safe to call as often as possible.

[usb_app.c](usb_app.c.md) calls `module_manager_list_modules()` when the PC sends command `0x10`.
