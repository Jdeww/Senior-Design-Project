/*
 * i2c_slave.h - MSSP I2C slave, handshake state machine, chain gate, register
 * map and event FIFO.
 *
 * Wiring it into the firmware:
 *   - call i2c_slave_init() at boot, then set RCONbits.IPEN and enable interrupts
 *   - in the high-priority ISR: if (PIR1bits.SSPIF) i2c_slave_isr();
 *   - call i2c_slave_task() on every main loop pass
 *
 * Registers auto-increment during a read, so several can be read in one
 * transaction. The event stream only runs when a read starts at REG_EVENTS.
 */
#ifndef I2C_SLAVE_H
#define I2C_SLAVE_H

#include <stdint.h>
#include <stdbool.h>
#include "../common/protocol.h"

#define I2C_SLAVE_SNAPSHOT_COUNT    SNAPSHOT_MAX

void i2c_slave_init(uint8_t input_type, uint8_t input_count);
void i2c_slave_isr(void);
void i2c_slave_task(void);

/* Main loop only. Returns false if the FIFO is full and the event was not queued. */
bool i2c_slave_push_event(uint8_t index, uint8_t value);

/* Main loop only. Sets the value of input n readable at REG_SNAPSHOT_BASE + n. */
void i2c_slave_set_snapshot(uint8_t input, uint8_t value);

#endif /* I2C_SLAVE_H */
