/*
 * daughter_db.h - The motherboard's EEPROM record of its own ID and of every
 * daughter board it has seen.
 *
 * Main loop only. The system tick must be running with interrupts enabled first.
 * Adding a board blocks for about 4 ms per changed byte. Updating a known board
 * whose details have not changed writes nothing.
 */
#ifndef DAUGHTER_DB_H
#define DAUGHTER_DB_H

#include <stdint.h>
#include <stdbool.h>

/* Call once at boot, before anything else here. Loads the stored ID and table.
 * On a blank or corrupt EEPROM, generates a new motherboard ID and starts an empty
 * table. Returns false if a write failed. The ID is still usable until the next
 * reset, but it may not have been saved. */
bool db_init(void);

uint16_t db_mb_id(void);

bool db_contains(uint16_t id);

/* Updates the entry for id, or adds one, replacing the oldest entry when the table
 * is full. Returns false if a write failed. */
bool db_save(uint16_t id, uint8_t type, uint8_t position);

#endif /* DAUGHTER_DB_H */
