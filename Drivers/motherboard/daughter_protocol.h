/*
 * daughter_protocol.h - The daughter boards' messages and registers as simple
 * calls, so the rest of the firmware never handles raw I2C bytes.
 *
 * Each call returns I2C_OK or an error from i2c_master.h. Two results need
 * particular handling by the caller:
 *   - I2C_NACK from dp_hello() is the normal end of the chain.
 *   - I2C_NACK from a position read counts toward NACK_LIMIT.
 */
#ifndef DAUGHTER_PROTOCOL_H
#define DAUGHTER_PROTOCOL_H

#include <stdint.h>
#include "i2c_master.h"

typedef struct {
    uint8_t  flag;      /* REPLY_FLAG_SEEN or REPLY_FLAG_NEW */
    uint16_t id;
    uint8_t  type;
} dp_reply_t;

typedef struct {
    uint8_t  type;
    uint8_t  count;
    uint8_t  fw_version;
    uint16_t id;
} dp_info_t;

typedef struct {
    uint8_t index;
    uint8_t value;
} dp_event_t;

/* Handshake, all sent to the one unregistered board on the bus. */
i2c_status_t dp_reset_chain(void);
i2c_status_t dp_hello(uint16_t mb_id);

/* Re-reads up to REPLY_RETRIES times on bad complement bytes, then returns
 * I2C_BAD_DATA and the caller restarts enumeration. */
i2c_status_t dp_read_reply(dp_reply_t *reply);

/* Returns I2C_BAD_DATA without touching the bus if pos is not 1..CHAIN_MAX_POSITIONS. */
i2c_status_t dp_accept(uint16_t id, uint8_t pos);
i2c_status_t dp_regenerate(uint16_t id);

/* Registered boards, addressed by chain position. */
i2c_status_t dp_read_info(uint8_t pos, dp_info_t *info);
i2c_status_t dp_read_snapshot(uint8_t pos, uint8_t input, uint8_t *value);

/* Reads up to max events into events[] and sets *count. Events beyond max stay
 * in the daughter's FIFO for the next poll. */
i2c_status_t dp_read_events(uint8_t pos, dp_event_t *events, uint8_t max, uint8_t *count);

#endif /* DAUGHTER_PROTOCOL_H */
