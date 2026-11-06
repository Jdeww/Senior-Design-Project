# eeprom.h

Source: `Drivers/common/eeprom.h`
Implementation: [eeprom.c](eeprom.c.md)

## What it does

Declares byte-level access to the chip's 256-byte data EEPROM, which keeps its contents through power-off. The motherboard uses it for the daughter database, and the real daughter identity storage can use it too.

## Inputs and outputs

| Function | Inputs | Output |
|---|---|---|
| `eeprom_read_byte(addr)` | Address, 0 to 255 | The stored byte |
| `eeprom_write_byte(addr, value)` | Address, 0 to 255; the byte to store | `true` if the byte now holds `value` |

## How it works

- Call these from the main loop only. The system tick must be running with interrupts enabled before any write, because the write timeout relies on it.
- A write blocks for about 4 ms per changed byte. Interrupts keep running during that wait, apart from a few cycles while the write starts.
- A byte that already holds `value` isn't rewritten, because each EEPROM cell can only be written a limited number of times.
