/*
 * eeprom.h - The chip's 256-byte data EEPROM, used on both boards.
 *
 * Main loop only. The system tick must be running with interrupts enabled before
 * any write, because the write timeout relies on it.
 *
 * A write blocks for about 4 ms per changed byte. Interrupts still run during
 * that wait, apart from a few cycles while the write is being started.
 */
#ifndef EEPROM_H
#define EEPROM_H

#include <stdint.h>
#include <stdbool.h>

uint8_t eeprom_read_byte(uint8_t addr);

/* Returns true if the byte now holds value. A byte that already holds value is
 * left alone, because each EEPROM cell can only be written a limited number of
 * times. */
bool eeprom_write_byte(uint8_t addr, uint8_t value);

#endif /* EEPROM_H */
