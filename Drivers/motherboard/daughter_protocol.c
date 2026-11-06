/*
 * daughter_protocol.c - See daughter_protocol.h.
 */
#include "daughter_protocol.h"
#include "../common/protocol.h"

#define ID_HI(id)   ((uint8_t)((id) >> 8))
#define ID_LO(id)   ((uint8_t)(id))

#define REG_INFO_LEN    5u      /* REG_INPUT_TYPE through REG_ID_LO */

static bool pos_is_valid(uint8_t pos)
{
    return (pos >= 1u) && (pos <= CHAIN_MAX_POSITIONS);
}

i2c_status_t dp_reset_chain(void)
{
    uint8_t msg[MSG_RESET_CHAIN_LEN];

    msg[0] = MSG_RESET_CHAIN;
    return i2c_write(I2C_ADDR_GENERAL_CALL, msg, MSG_RESET_CHAIN_LEN);
}

i2c_status_t dp_hello(uint16_t mb_id)
{
    uint8_t msg[MSG_HELLO_LEN];

    msg[0] = MSG_HELLO;
    msg[1] = ID_HI(mb_id);
    msg[2] = ID_LO(mb_id);
    return i2c_write(I2C_ADDR_DISCOVERY, msg, MSG_HELLO_LEN);
}

static bool reply_is_valid(const uint8_t *raw)
{
    uint8_t i;

    for (i = 0u; i < REPLY_IDX_COMPLEMENT; i++) {
        if ((uint8_t)(raw[i] ^ raw[REPLY_IDX_COMPLEMENT + i]) != 0xFFu) {
            return false;
        }
    }
    return (raw[REPLY_IDX_FLAG] == REPLY_FLAG_SEEN) || (raw[REPLY_IDX_FLAG] == REPLY_FLAG_NEW);
}

i2c_status_t dp_read_reply(dp_reply_t *reply)
{
    uint8_t raw[REPLY_LEN];
    uint8_t attempt;

    for (attempt = 0u; attempt <= REPLY_RETRIES; attempt++) {
        i2c_status_t status = i2c_read(I2C_ADDR_DISCOVERY, raw, REPLY_LEN);

        if (status != I2C_OK) {
            return status;
        }
        if (reply_is_valid(raw)) {
            reply->flag = raw[REPLY_IDX_FLAG];
            reply->id = (uint16_t)(((uint16_t)raw[REPLY_IDX_ID_HI] << 8) | raw[REPLY_IDX_ID_LO]);
            reply->type = raw[REPLY_IDX_TYPE];
            return I2C_OK;
        }
    }
    return I2C_BAD_DATA;
}

i2c_status_t dp_accept(uint16_t id, uint8_t pos)
{
    uint8_t msg[MSG_ACCEPT_LEN];

    if (!pos_is_valid(pos)) {
        return I2C_BAD_DATA;
    }
    msg[0] = MSG_ACCEPT;
    msg[1] = ID_HI(id);
    msg[2] = ID_LO(id);
    msg[3] = pos;
    return i2c_write(I2C_ADDR_DISCOVERY, msg, MSG_ACCEPT_LEN);
}

i2c_status_t dp_regenerate(uint16_t id)
{
    uint8_t msg[MSG_REGENERATE_LEN];

    msg[0] = MSG_REGENERATE;
    msg[1] = ID_HI(id);
    msg[2] = ID_LO(id);
    return i2c_write(I2C_ADDR_DISCOVERY, msg, MSG_REGENERATE_LEN);
}

i2c_status_t dp_read_info(uint8_t pos, dp_info_t *info)
{
    uint8_t raw[REG_INFO_LEN];
    i2c_status_t status;

    if (!pos_is_valid(pos)) {
        return I2C_BAD_DATA;
    }
    /* The daughter's register pointer auto-increments across this read. */
    status = i2c_read_reg(I2C_ADDR_FOR_POS(pos), REG_INPUT_TYPE, raw, REG_INFO_LEN);
    if (status != I2C_OK) {
        return status;
    }
    info->type = raw[REG_INPUT_TYPE];
    info->count = raw[REG_INPUT_COUNT];
    info->fw_version = raw[REG_FW_VERSION];
    info->id = (uint16_t)(((uint16_t)raw[REG_ID_HI] << 8) | raw[REG_ID_LO]);
    return I2C_OK;
}

i2c_status_t dp_read_snapshot(uint8_t pos, uint8_t input, uint8_t *value)
{
    if (!pos_is_valid(pos)) {
        return I2C_BAD_DATA;
    }
    return i2c_read_reg(I2C_ADDR_FOR_POS(pos), (uint8_t)(REG_SNAPSHOT_BASE + input), value, 1u);
}

i2c_status_t dp_read_events(uint8_t pos, dp_event_t *events, uint8_t max, uint8_t *count)
{
    i2c_status_t status;
    uint8_t available = 0u;
    uint8_t wanted;
    uint8_t i;

    *count = 0u;
    if (!pos_is_valid(pos)) {
        return I2C_BAD_DATA;
    }

    status = i2c_begin_read_reg(I2C_ADDR_FOR_POS(pos), REG_EVENTS);
    if (status != I2C_OK) {
        return status;      /* i2c_begin_read_reg() already cleaned up */
    }

    /* The count byte decides whether to ACK, so look at it first. A count the
     * FIFO could never hold means the read is corrupt: end it without taking
     * any events, so nothing is lost. */
    status = i2c_receive(&available);
    if (status != I2C_OK) {
        return i2c_abort(status);
    }
    if (available > EVENT_FIFO_DEPTH) {
        wanted = 0u;
    } else {
        wanted = (available < max) ? available : max;
    }
    status = i2c_acknowledge(wanted > 0u);

    for (i = 0u; (status == I2C_OK) && (i < wanted); i++) {
        status = i2c_receive(&events[i].index);
        if (status == I2C_OK) {
            status = i2c_acknowledge(true);
        }
        if (status == I2C_OK) {
            status = i2c_receive(&events[i].value);
        }
        if (status == I2C_OK) {
            status = i2c_acknowledge((uint8_t)(i + 1u) < wanted);
        }
    }
    if (status == I2C_OK) {
        status = i2c_stop();
    }
    if (status != I2C_OK) {
        return i2c_abort(status);
    }

    *count = wanted;
    return (available > EVENT_FIFO_DEPTH) ? I2C_BAD_DATA : I2C_OK;
}
