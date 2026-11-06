/*
 * chain.h - Registers daughter boards one at a time, in physical chain order, and
 * keeps the table of who is at each position.
 *
 * Main loop only. Call db_init() first. A new board blocks the main loop while
 * its database entry is written, about 24 ms.
 */
#ifndef CHAIN_H
#define CHAIN_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t id;
    uint8_t  type;
    uint8_t  input_count;
    uint8_t  fw_version;
} chain_board_t;

typedef enum {
    CHAIN_NO_CHANGE = 0,    /* no new board at the end of the chain   */
    CHAIN_ADDED,            /* one new board was added                */
    CHAIN_ERROR             /* something failed; call chain_enumerate() */
} chain_append_t;

/* Resets the chain and registers every board from position 1, retrying the whole
 * pass if a step fails. Returns true if the pass finished, or false if every try
 * failed, leaving an empty table. Call again later to retry. */
bool chain_enumerate(void);

/* Looks for one new board at the end of the chain, without disturbing the others.
 * Does nothing when the chain is full. Call every HELLO_INTERVAL_MS. */
chain_append_t chain_append(void);

uint8_t chain_count(void);

/* Returns the board at position 1..chain_count(), or a null pointer if there is
 * none. */
const chain_board_t *chain_get(uint8_t position);

#endif /* CHAIN_H */
