/*
 * identity.h - What the I2C slave needs from identity storage. The real EEPROM
 * version is not written yet; identity_stub.c stands in for it during bring-up.
 *
 * The slave calls these from i2c_slave_task(), in the main loop. The motherboard
 * is waiting on the clock during the lookup and new-ID calls, so they must finish
 * well inside I2C_TIMEOUT_MS. identity_save() writes EEPROM, which is slow, so
 * it should queue the write rather than block.
 */
#ifndef IDENTITY_H
#define IDENTITY_H

#include <stdint.h>

/* Returns the ID to use with this motherboard. Sets *flag to REPLY_FLAG_SEEN
 * with the stored ID, or REPLY_FLAG_NEW with a fresh random ID. */
uint16_t identity_lookup(uint16_t mb_id, uint8_t *flag);

/* Returns a fresh random ID, never ID_INVALID_LOW or ID_INVALID_HIGH. */
uint16_t identity_new_id(void);

/* Remembers the pair so the next HELLO from this motherboard answers "seen". */
void identity_save(uint16_t mb_id, uint16_t id);

#endif /* IDENTITY_H */
