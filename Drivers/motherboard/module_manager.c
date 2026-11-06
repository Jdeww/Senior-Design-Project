/*
 * module_manager.c - See module_manager.h.
 *
 * Two rules keep the PC's view consistent:
 *
 * - A board is only polled once it has been announced, so the PC always gets its
 *   attach report before any of its events.
 *
 * - Events are only read when the queue has room for them. Otherwise they wait in
 *   the daughter's FIFO, so a slow PC does not lose them. Polling continues with
 *   room for none, because the poll is also how a dropped board is noticed.
 */
#include "module_manager.h"
#include "chain.h"
#include "daughter_db.h"
#include "daughter_protocol.h"
#include "report_queue.h"
#include "../common/protocol.h"
#include "../common/system_tick.h"

#define MAX_HELD_REPORTS    (CHAIN_MAX_POSITIONS * 8u)

/* A restart must always fit: held key releases, a detach per board, chain reset. */
#if REPORT_QUEUE_DEPTH < (MAX_HELD_REPORTS + CHAIN_MAX_POSITIONS + 1u)
    #error "module_manager: report queue is too small for a chain restart"
#endif

static uint8_t  s_held[CHAIN_MAX_POSITIONS];        /* one bit per held input   */
static uint8_t  s_failures[CHAIN_MAX_POSITIONS];    /* failed polls in a row    */
static uint8_t  s_announced;                        /* boards the PC knows about */
static bool     s_complete_due;
static bool     s_needs_enumerating;
static uint16_t s_last_poll;
static uint16_t s_last_hello;

static dp_event_t s_events[EVENT_FIFO_DEPTH];

static uint8_t snapshots_to_send(const chain_board_t *board)
{
    if ((board->type != INPUT_TYPE_SLIDER) && (board->type != INPUT_TYPE_COCKPIT)) {
        return 0u;
    }
    return (board->input_count < SNAPSHOT_MAX) ? board->input_count : SNAPSHOT_MAX;
}

/* Sends an attach report for each new board, followed by the current state of its
 * sliders and switches so the PC starts in sync. A board waits until all of its
 * reports fit. */
static void announce_boards(void)
{
    const chain_board_t *board;
    uint8_t snapshots;
    uint8_t position;
    uint8_t value;
    uint8_t i;

    while (s_announced < chain_count()) {
        position = (uint8_t)(s_announced + 1u);
        board = chain_get(position);
        snapshots = snapshots_to_send(board);
        if (report_queue_free() < (uint8_t)(snapshots + 1u)) {
            return;
        }

        (void)report_push_attach(position, board->id, board->type, board->input_count);
        for (i = 0u; i < snapshots; i++) {
            if (dp_read_snapshot(position, i, &value) == I2C_OK) {
                (void)report_push_input(position, i, value);
            }
        }
        s_announced = position;
    }

    if (s_complete_due && (report_queue_free() > 0u)) {
        (void)report_push_chain_complete(chain_count());
        s_complete_due = false;
    }
}

static void run_enumeration(void)
{
    uint8_t i;

    for (i = 0u; i < CHAIN_MAX_POSITIONS; i++) {
        s_held[i] = 0u;
        s_failures[i] = 0u;
    }
    s_announced = 0u;

    s_needs_enumerating = !chain_enumerate();
    s_complete_due = !s_needs_enumerating;
}

/* Tells the PC the layout is going away, then rebuilds it. Queued reports are only
 * dropped if there's no room, because the chain reset replaces what they describe
 * and the slider and switch states are sent again afterward. */
static void restart_chain(void)
{
    uint8_t needed = 1u;    /* the chain reset */
    uint8_t position;
    uint8_t held;
    uint8_t bit;

    for (position = 1u; position <= s_announced; position++) {
        held = s_held[position - 1u];
        while (held != 0u) {
            needed = (uint8_t)(needed + (held & 1u));
            held >>= 1;
        }
        if (s_failures[position - 1u] >= NACK_LIMIT) {
            needed++;
        }
    }
    if (report_queue_free() < needed) {
        report_queue_clear();
    }

    /* Release every held input on every board, not just the failed one, because
     * positions are about to change and the PC is clearing its layout. */
    for (position = 1u; position <= s_announced; position++) {
        held = s_held[position - 1u];
        for (bit = 0u; bit < 8u; bit++) {
            if ((held & (uint8_t)(1u << bit)) != 0u) {
                (void)report_push_input(position, bit, 0u);
            }
        }
    }

    /* Removing a board also cuts off every board behind it, so several can fail. */
    for (position = 1u; position <= s_announced; position++) {
        if (s_failures[position - 1u] >= NACK_LIMIT) {
            (void)report_push_detach(position, chain_get(position)->id);
        }
    }

    (void)report_push_chain_reset();
    run_enumeration();
}

/* Remembers which inputs are held, so they can be released if the board drops out.
 * Only key switches and the encoder's button are held. Sliders and cockpit switches
 * stay where they are set, and the encoder's detent count isn't a pressed state. */
static void track_held(uint8_t position, uint8_t type, uint8_t index, uint8_t value)
{
    uint8_t mask;

    if (index >= 8u) {
        return;
    }
    if ((type == INPUT_TYPE_KEYS)
            || ((type == INPUT_TYPE_ENCODER) && (index == ENCODER_IDX_BUTTON))) {
        mask = (uint8_t)(1u << index);
        if (value != 0u) {
            s_held[position - 1u] |= mask;
        } else {
            s_held[position - 1u] &= (uint8_t)~mask;
        }
    }
}

static void poll_boards(void)
{
    i2c_status_t status;
    uint8_t position;
    uint8_t room;
    uint8_t count;
    uint8_t type;
    uint8_t i;
    bool board_lost = false;

    for (position = 1u; position <= s_announced; position++) {
        room = report_queue_free();
        if (room > EVENT_FIFO_DEPTH) {
            room = EVENT_FIFO_DEPTH;
        }

        status = dp_read_events(position, s_events, room, &count);

        /* The daughter has already removed these, even if the read then failed. */
        type = chain_get(position)->type;
        for (i = 0u; i < count; i++) {
            track_held(position, type, s_events[i].index, s_events[i].value);
            (void)report_push_input(position, s_events[i].index, s_events[i].value);
        }

        /* A board unplugged part way through a read can give a timeout or a corrupt
         * count instead of a NACK, so any failure counts. */
        if (status == I2C_OK) {
            s_failures[position - 1u] = 0u;
        } else {
            s_failures[position - 1u]++;
            if (s_failures[position - 1u] >= NACK_LIMIT) {
                board_lost = true;
            }
        }
    }

    if (board_lost) {
        restart_chain();
    }
}

void module_manager_init(void)
{
    /* If a write fails, the ID still works until the next reset, and the decision
     * table copes with a database that is out of date. */
    (void)db_init();

    report_queue_clear();
    (void)report_push_chain_reset();
    run_enumeration();

    s_last_poll = tick_now();
    s_last_hello = s_last_poll;
}

void module_manager_task(void)
{
    uint16_t now = tick_now();

    announce_boards();

    if ((uint16_t)(now - s_last_poll) >= POLL_INTERVAL_MS) {
        s_last_poll = now;
        poll_boards();
    }

    if ((uint16_t)(now - s_last_hello) >= HELLO_INTERVAL_MS) {
        s_last_hello = now;
        if (s_needs_enumerating) {
            run_enumeration();
        } else if (chain_append() == CHAIN_ERROR) {
            restart_chain();
        }
    }
}
