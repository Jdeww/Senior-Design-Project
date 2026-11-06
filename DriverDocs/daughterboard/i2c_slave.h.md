# i2c_slave.h

Source: `Drivers/daughterboard/i2c_slave.h`
Implementation: [i2c_slave.c](i2c_slave.c.md)

## What it does

Defines how the rest of the daughter firmware uses the I2C slave driver: starting it, connecting it to the interrupt and main loop, and handing it events and input values for the motherboard to collect.

## Inputs and outputs

| Function | Inputs | Output | Call from |
|---|---|---|---|
| `i2c_slave_init(input_type, input_count)` | Input type code; number of inputs | None | Boot |
| `i2c_slave_isr()` | None | None | High-priority interrupt |
| `i2c_slave_task()` | None | None | Every main loop pass |
| `i2c_slave_push_event(index, value)` | Which input; its new value | `true` if queued, `false` if the queue is full | Main loop only |
| `i2c_slave_set_snapshot(input, value)` | Input number; its current value | None. Inputs out of range are ignored. | Main loop only |

`I2C_SLAVE_SNAPSHOT_COUNT` is 8, which provides snapshot registers 0x20 to 0x27.

## How it works

### Connecting it to the firmware

```c
void __interrupt(high_priority) high_isr(void)
{
    if (PIR1bits.SSPIF) {
        i2c_slave_isr();
    }
}

void main(void)
{
    i2c_slave_init(INPUT_TYPE_KEYS, INPUT_COUNT_KEYS);
    RCONbits.IPEN = 1;          /* turn on interrupt priorities */
    INTCONbits.GIEH = 1;
    INTCONbits.GIEL = 1;

    for (;;) {
        i2c_slave_task();
        /* run the input driver, which calls i2c_slave_push_event() */
    }
}
```

`i2c_slave_init()` enables the MSSP interrupt, but the caller must turn on interrupts globally.

### Register reads

Registers auto-increment during a read, so several can be read in one transaction. The event stream only runs when a read *starts* at `REG_EVENTS`. A read that reaches that register by incrementing gets the event count and removes nothing from the queue.

If `i2c_slave_push_event()` returns `false`, the event was not queued, so the caller needs to try again or drop it.
