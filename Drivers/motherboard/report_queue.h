/*
 * report_queue.h - Reports waiting to go to the PC, oldest first.
 *
 * The PC only collects a report when it polls, once every 1 ms, so bursts wait
 * here. The USB driver will drain this with report_queue_pop().
 *
 * Main loop only. The push functions return false, and add nothing, when the
 * queue is full.
 */
#ifndef REPORT_QUEUE_H
#define REPORT_QUEUE_H

#include <stdint.h>
#include <stdbool.h>

#define REPORT_QUEUE_DEPTH  40u

void report_queue_clear(void);
uint8_t report_queue_free(void);

bool report_push_input(uint8_t position, uint8_t index, uint8_t value);
bool report_push_attach(uint8_t position, uint16_t id, uint8_t type, uint8_t input_count);
bool report_push_detach(uint8_t position, uint16_t id);
bool report_push_chain_reset(void);
bool report_push_chain_complete(uint8_t board_count);

/* Copies the oldest report into out, which must hold USB_REPORT_LEN bytes, with
 * unused bytes set to 0. Returns false if the queue is empty. */
bool report_queue_pop(uint8_t *out);

#endif /* REPORT_QUEUE_H */
