# system_tick.c

Source: `Drivers/common/system_tick.c`
Interface: [system_tick.h](system_tick.h.md)

## What it does

Sets up Timer2 to interrupt every 1 ms and counts those interrupts in `g_ms`. The same file builds for both boards, choosing its timer settings from the board's clock speed.

## Inputs and outputs

The function-level inputs and outputs are listed in [system_tick.h](system_tick.h.md). This file also interacts with:

| Direction | Item | Use |
|---|---|---|
| Input | `_XTAL_FREQ` from `board.h` | Selects the timer settings at compile time |
| Input | `TICK_MS` from `protocol.h` | The required period, used in a build-time check |
| Input | `PIR1.TMR2IF` | Set by the hardware at each period; the board's interrupt handler checks it |
| Output | `T2CON`, `PR2`, `TMR2` | Timer2 setup |
| Output | `IPR1.TMR2IP`, `PIE1.TMR2IE` | Sets the interrupt to low priority and enables it |
| Output | `g_ms` | The millisecond count |

Because `board.h` differs by board, each build's include path must contain that board's folder (`Drivers/motherboard` or `Drivers/daughterboard`).

## How it works

### Timer settings

Timer2 counts instruction cycles, which run at a quarter of the CPU clock:

```text
tick rate = Fosc / 4 / prescale / (PR2 + 1) / postscale
```

| Board | Fosc | Instruction clock | Prescale | PR2 + 1 | Postscale | Tick rate |
|---|---|---|---|---|---|---|
| Motherboard | 48 MHz | 12 MHz | 16 | 250 | 3 | 1000 Hz |
| Daughter | 16 MHz | 4 MHz | 4 | 250 | 4 | 1000 Hz |

The build stops with an `#error` if the clock speed has no settings, or if the settings don't work out to `TICK_MS`.

### `tick_init()`

1. Stops Timer2 and clears its count.
2. Sets the period, prescaler and postscaler.
3. Clears the interrupt flag, sets low priority and enables the interrupt.
4. Starts the timer.

### `tick_isr()`

Clears `TMR2IF` and adds 1 to `g_ms`. It does nothing else, so the interrupt stays short.

### `tick_now()`

Reads `g_ms` twice and repeats until both reads match. A tick between the two bytes of a read could otherwise combine an old low byte with a new high byte. Reading this way avoids switching interrupts off, which would delay the tick.

## Testing

The period was measured in the MPLAB X simulator (v6.35) with a test program that runs only the tick, stopping at each tick for several ticks in a row:

| Board build | Expected cycles per tick | Measured |
|---|---|---|
| Motherboard | 12,000 | 12,000 |
| Daughter | 4,000 | 4,000, with one reading of 3,999 |

The 3,999 is consistent with normal interrupt timing, since the interrupt can't start until the current instruction finishes. On both builds, `g_ms` read 3 when stopped at the fourth tick, just before that tick added 1, so the count advances once per tick.

This confirms the timer settings in instruction cycles. It doesn't confirm real-world time: 1 ms also requires the CPU to actually run at `_XTAL_FREQ`, which [clock.c](clock.c.md) sets up but only real hardware can confirm. See the scope test in the [motherboard main.c page](../motherboard/main.c.md).
