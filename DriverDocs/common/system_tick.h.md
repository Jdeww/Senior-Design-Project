# system_tick.h

Source: `Drivers/common/system_tick.h`
Implementation: [system_tick.c](system_tick.c.md)

## What it does

Defines the 1 ms time base shared by both boards. The firmware uses it for timing without delay loops, including the motherboard's I2C timeouts.

## Inputs and outputs

| Item | Inputs | Output | Use from |
|---|---|---|---|
| `tick_init()` | None | None. Sets up and starts Timer2. | Boot |
| `tick_isr()` | None | None. Clears the flag and adds 1 to `g_ms`. | Low-priority interrupt |
| `tick_now()` | None | Milliseconds since boot, as `uint16_t` | Main loop only |
| `g_ms` | Not applicable | `volatile uint16_t`, milliseconds since boot | Read in the main loop; written only by `tick_isr()` |

## How it works

### Connecting it to the firmware

```c
void __interrupt(low_priority) low_isr(void)
{
    if (PIR1bits.TMR2IF) {
        tick_isr();
    }
}

void main(void)
{
    tick_init();
    RCONbits.IPEN = 1;          /* turn on interrupt priorities */
    INTCONbits.GIEH = 1;
    INTCONbits.GIEL = 1;

    /* main loop */
}
```

`tick_init()` enables the Timer2 interrupt, but the caller must turn on interrupts globally. On the motherboard, this must happen before any I2C call.

### Measuring time

The counter wraps about every 65.5 seconds. Subtracting two readings still gives the right elapsed time across a wrap, as long as the interval is shorter than that:

```c
if ((uint16_t)(tick_now() - last_poll) >= POLL_INTERVAL_MS) {
    last_poll = tick_now();
    /* poll the boards */
}
```

### Reading `g_ms` directly

`g_ms` takes two reads on this 8-bit chip, so a plain read in the main loop can return a wrong value if a tick lands in between. Use `tick_now()` for the full value. Casting to `uint8_t` is also safe, because a single byte can't change partway through being read. The I2C timeouts do this.
