/*
 * report_queue.c - See report_queue.h.
 *
 * Reports are 8 bytes on USB, but none uses more than 6, so only 6 are stored.
 */
#include "motherboard/report_queue.h"
#include "common/protocol.h"

#define STORED_LEN      6u

static uint8_t s_reports[REPORT_QUEUE_DEPTH][STORED_LEN];
static uint8_t s_head;      /* slot the next push uses       */
static uint8_t s_tail;      /* slot holding the oldest report */
static uint8_t s_count;

void report_queue_clear(void)
{
    s_head = 0u;
    s_tail = 0u;
    s_count = 0u;
}

uint8_t report_queue_free(void)
{
    return (uint8_t)(REPORT_QUEUE_DEPTH - s_count);
}

static bool push(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5)
{
    uint8_t *slot;

    if (s_count >= REPORT_QUEUE_DEPTH) {
        return false;
    }
    slot = s_reports[s_head];
    slot[0] = b0;
    slot[1] = b1;
    slot[2] = b2;
    slot[3] = b3;
    slot[4] = b4;
    slot[5] = b5;

    s_head++;
    if (s_head >= REPORT_QUEUE_DEPTH) {
        s_head = 0u;
    }
    s_count++;
    return true;
}

bool report_push_input(uint8_t position, uint8_t index, uint8_t value)
{
    return push(RPT_INPUT_EVENT, position, index, value, 0u, 0u);
}

bool report_push_attach(uint8_t position, uint16_t id, uint8_t type, uint8_t input_count)
{
    return push(RPT_ATTACH, position, (uint8_t)(id >> 8), (uint8_t)id, type, input_count);
}

bool report_push_detach(uint8_t position, uint16_t id)
{
    return push(RPT_DETACH, position, (uint8_t)(id >> 8), (uint8_t)id, 0u, 0u);
}

bool report_push_chain_reset(void)
{
    return push(RPT_CHAIN_RESET, 0u, 0u, 0u, 0u, 0u);
}

bool report_push_chain_complete(uint8_t board_count)
{
    return push(RPT_CHAIN_COMPLETE, board_count, 0u, 0u, 0u, 0u);
}

bool report_queue_pop(uint8_t *out)
{
    const uint8_t *slot;
    uint8_t i;

    if (s_count == 0u) {
        return false;
    }
    slot = s_reports[s_tail];
    for (i = 0u; i < USB_REPORT_LEN; i++) {
        out[i] = (i < STORED_LEN) ? slot[i] : 0u;
    }

    s_tail++;
    if (s_tail >= REPORT_QUEUE_DEPTH) {
        s_tail = 0u;
    }
    s_count--;
    return true;
}
