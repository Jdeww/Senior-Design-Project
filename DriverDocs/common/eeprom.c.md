# eeprom.c

Source: `Drivers/common/eeprom.c`
Interface: [eeprom.h](eeprom.h.md)

## What it does

Reads and writes single bytes of the chip's data EEPROM, skipping writes that wouldn't change anything and checking each write afterward.

## Inputs and outputs

The function-level inputs and outputs are listed in [eeprom.h](eeprom.h.md). This file also interacts with:

| Direction | Item | Use |
|---|---|---|
| Input | `g_ms` | Time limit on writes |
| Input | `EEDATA` | The byte that was read |
| Input | `EECON1.WR` | Stays set until the hardware finishes a write |
| Output | `EEADR`, `EEDATA` | The address and the byte to write |
| Output | `EECON1`, `EECON2` | Start reads and writes |
| Output | `INTCON.GIEH` | Holds off all interrupts during the write's unlock sequence |

## How it works

### Reading

Sets the address, selects data EEPROM rather than program memory, sets `RD`, and returns `EEDATA`.

### Writing

1. Waits for any earlier write to finish.
2. Reads the byte. If it already holds `value`, returns `true` with no write.
3. Sets the address and data, and enables writes (`WREN`).
4. Turns interrupts off, writes `0x55` then `0xAA` to `EECON2`, sets `WR`, then restores interrupts to how they were.
5. Waits up to 10 ms for `WR` to clear. Interrupts run normally during this wait.
6. Disables writes, reads the byte back, and returns `true` if it matches.

Step 4 is the hardware's protection against accidental writes: it only accepts a write straight after that exact sequence, so nothing may interrupt it.

## Testing

| Check | Method | Result |
|---|---|---|
| Unlock sequence | Read the compiled assembly | `55h`, `AAh` and `WR` are consecutive with interrupts off |
| Write, overwrite, read back | MPLAB X simulator | Pass |
| Interrupts restored after a write | MPLAB X simulator | Pass |
| Real write timing and cell behaviour | Not tested | The simulator finishes writes much faster than real hardware |
