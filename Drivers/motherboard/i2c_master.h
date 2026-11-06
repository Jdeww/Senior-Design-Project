/*
 * i2c_master.h - MSSP I2C master and bus recovery.
 *
 * Every wait has a timeout, so a bad pogo contact cannot freeze the main loop.
 * Call these from the main loop only, never from an interrupt. The system tick
 * must be running with interrupts enabled first, because the timeouts rely on it.
 *
 * The whole-transaction functions clean up after themselves on failure:
 *   I2C_NACK                 sends STOP. The bus is healthy, nobody answered.
 *   I2C_TIMEOUT, COLLISION   runs bus recovery, then returns the original error.
 *   I2C_BUS_STUCK            recovery failed, a line is still held low.
 */
#ifndef I2C_MASTER_H
#define I2C_MASTER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    I2C_OK = 0,
    I2C_NACK,           /* no ACK; expected from HELLO at the end of the chain */
    I2C_TIMEOUT,
    I2C_COLLISION,
    I2C_BUS_STUCK,
    I2C_BAD_DATA        /* transfer worked but the contents failed a check     */
} i2c_status_t;

void i2c_init(void);

/* Clocks SCL up to 9 times until SDA is released, sends a manual STOP, and
 * re-initializes the MSSP. Returns I2C_OK or I2C_BUS_STUCK. */
i2c_status_t i2c_bus_recover(void);

/* Whole transactions. addr is the 7-bit address. */
i2c_status_t i2c_write(uint8_t addr, const uint8_t *data, uint8_t len);
i2c_status_t i2c_read(uint8_t addr, uint8_t *data, uint8_t len);
i2c_status_t i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len);

/*
 * Pieces of a register read, for when the length is only known part way
 * through (the event stream). After i2c_begin_read_reg() succeeds:
 *   - for each byte, call i2c_receive(), look at the byte if needed, then call
 *     i2c_acknowledge(true), or i2c_acknowledge(false) for the last byte
 *   - finish with i2c_stop()
 *   - on any error, pass the status to i2c_abort() instead
 */
i2c_status_t i2c_begin_read_reg(uint8_t addr, uint8_t reg);
i2c_status_t i2c_receive(uint8_t *out);
i2c_status_t i2c_acknowledge(bool more);
i2c_status_t i2c_stop(void);
i2c_status_t i2c_abort(i2c_status_t reason);

#endif /* I2C_MASTER_H */
