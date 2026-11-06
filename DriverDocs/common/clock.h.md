# clock.h

Source: `Drivers/common/clock.h`
Implementation: [clock.c](clock.c.md)

## What it does

Declares the one function that starts each board's oscillator. Including the matching `clock.c` in a build also sets that chip's configuration bits.

## Inputs and outputs

| Function | Inputs | Output |
|---|---|---|
| `clock_init()` | None | `true` once the clock is stable at `_XTAL_FREQ`; `false` if it never settled |

## How it works

Call it first in `main`, because every timing in the firmware assumes the final clock speed. If it returns `false`, the caller should stop: USB and the tick can't be trusted at the wrong speed.

```c
if (!clock_init()) {
    for (;;) {
    }
}
```
