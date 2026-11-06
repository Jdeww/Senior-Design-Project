/*
 * chain.c - See chain.h.
 *
 * Only one unregistered board can hear the bus at a time, because each board's
 * gate stays open until it is accepted. Registering boards one by one therefore
 * gives their positions in physical order.
 */
#include "board.h"
#include "chain.h"
#include "daughter_db.h"
#include "daughter_protocol.h"
#include "../common/protocol.h"
#include "../common/system_tick.h"

#define ENUMERATE_ATTEMPTS  3u  /* full passes before giving up until the next call */
#define REGENERATE_LIMIT    5u  /* new IDs to ask one board for before giving up    */

typedef enum { STEP_ADDED, STEP_END_OF_CHAIN, STEP_FAILED } step_t;

static chain_board_t s_boards[CHAIN_MAX_POSITIONS];
static uint8_t s_count;

/* Waits at least ms. The next tick can arrive almost immediately, hence the >. */
static void wait_ms(uint8_t ms)
{
    uint8_t start = (uint8_t)g_ms;

    while ((uint8_t)((uint8_t)g_ms - start) <= ms) {
    }
}

static bool id_in_chain(uint16_t id)
{
    uint8_t i;

    for (i = 0u; i < s_count; i++) {
        if (s_boards[i].id == id) {
            return true;
        }
    }
    return false;
}

/* The decision table from the Firmware Driver Summary, plus a check for the IDs
 * a board must never use. A reply reaching this point is known to be intact:
 *
 *   Flag     In database   In chain   Decision
 *   either   -             yes        REGENERATE: two boards claim one ID
 *   new      yes           no         REGENERATE: random ID clashed with a known board
 *   new      no            no         ACCEPT, add
 *   seen     yes           no         ACCEPT, update
 *   seen     no            no         ACCEPT, re-add
 */
static bool should_accept(const dp_reply_t *reply)
{
    if ((reply->id == ID_INVALID_LOW) || (reply->id == ID_INVALID_HIGH)) {
        return false;
    }
    if (id_in_chain(reply->id)) {
        return false;
    }
    return !((reply->flag == REPLY_FLAG_NEW) && db_contains(reply->id));
}

static step_t register_board(uint8_t position)
{
    dp_reply_t reply;
    dp_info_t info;
    i2c_status_t status;
    uint8_t regenerated = 0u;

    status = dp_hello(db_mb_id());
    if (status == I2C_NACK) {
        return STEP_END_OF_CHAIN;
    }
    if (status != I2C_OK) {
        return STEP_FAILED;
    }

    for (;;) {
        if (dp_read_reply(&reply) != I2C_OK) {
            return STEP_FAILED;
        }
        if (should_accept(&reply)) {
            break;
        }
        if ((regenerated >= REGENERATE_LIMIT) || (dp_regenerate(reply.id) != I2C_OK)) {
            return STEP_FAILED;
        }
        regenerated++;
    }

    if (dp_accept(reply.id, position) != I2C_OK) {
        return STEP_FAILED;
    }
    wait_ms(GATE_SETTLE_MS);

    /* Confirms that this board really moved to its new address. */
    if ((dp_read_info(position, &info) != I2C_OK) || (info.id != reply.id)) {
        return STEP_FAILED;
    }

    s_boards[position - 1u].id = reply.id;
    s_boards[position - 1u].type = info.type;
    s_boards[position - 1u].input_count = info.count;
    s_boards[position - 1u].fw_version = info.fw_version;
    s_count = position;

    /* A failed save only means this board may look new next time, which the
     * decision table handles, so it should not stop the board being used. */
    (void)db_save(reply.id, info.type, position);

    return STEP_ADDED;
}

static bool enumerate_once(void)
{
    i2c_status_t status;
    step_t step;

    s_count = 0u;

    /* A NACK means no boards are connected, which is not a failure. */
    status = dp_reset_chain();
    if ((status != I2C_OK) && (status != I2C_NACK)) {
        return false;
    }
    wait_ms(GATE_SETTLE_MS);

    while (s_count < CHAIN_MAX_POSITIONS) {
        step = register_board((uint8_t)(s_count + 1u));
        if (step == STEP_END_OF_CHAIN) {
            return true;
        }
        if (step == STEP_FAILED) {
            s_count = 0u;
            return false;
        }
    }
    return true;
}

bool chain_enumerate(void)
{
    uint8_t attempt;

    for (attempt = 0u; attempt < ENUMERATE_ATTEMPTS; attempt++) {
        if (enumerate_once()) {
            return true;
        }
    }
    return false;
}

chain_append_t chain_append(void)
{
    step_t step;

    if (s_count >= CHAIN_MAX_POSITIONS) {
        return CHAIN_NO_CHANGE;
    }

    step = register_board((uint8_t)(s_count + 1u));
    if (step == STEP_ADDED) {
        return CHAIN_ADDED;
    }
    return (step == STEP_END_OF_CHAIN) ? CHAIN_NO_CHANGE : CHAIN_ERROR;
}

uint8_t chain_count(void)
{
    return s_count;
}

const chain_board_t *chain_get(uint8_t position)
{
    if ((position < 1u) || (position > s_count)) {
        return 0;
    }
    return &s_boards[position - 1u];
}
