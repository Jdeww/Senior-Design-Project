/*
 * i2c_master.c - MSSP I2C master and bus recovery. See i2c_master.h.
 */
#include "board.h"
#include "i2c_master.h"
#include "../common/protocol.h"
#include "../common/system_tick.h"

/* Fscl = Fosc / (4 * (SSP1ADD + 1)) */
#define I2C_BRG             ((uint8_t)((_XTAL_FREQ / (4UL * I2C_BUS_HZ)) - 1UL))

#define SSPCON1_MASTER      0x28u   /* SSPEN = 1, SSPM = 1000 (I2C master)    */
#define SSPSTAT_100KHZ      0x80u   /* SMP = 1: slew control off below 400 kHz */

#define ADDR_WRITE(a)       ((uint8_t)((uint8_t)((a) << 1)))
#define ADDR_READ(a)        ((uint8_t)((uint8_t)((a) << 1) | 1u))

#define RECOVERY_HALF_US    5u      /* half of a 100 kHz clock period         */

void i2c_init(void)
{
    SSP1CON1 = 0;

    I2C_SDA_ANS = 0;
    I2C_SCL_ANS = 0;
    I2C_SDA_TRIS = 1;
    I2C_SCL_TRIS = 1;

    SSP1STAT = SSPSTAT_100KHZ;
    SSP1CON2 = 0;
    SSP1CON3 = 0;
    SSP1ADD = I2C_BRG;

    PIR1bits.SSPIF = 0;
    PIR2bits.BCLIF = 0;

    SSP1CON1 = SSPCON1_MASTER;
}

/* Waits for the current MSSP operation to finish.
 *
 * Compares against > rather than >= because the first tick can arrive almost
 * immediately. This guarantees at least I2C_TIMEOUT_MS of real waiting, which
 * also gives a daughter room to stretch the clock. */
static i2c_status_t wait_done(void)
{
    uint8_t start = (uint8_t)g_ms;      /* the low byte alone cannot tear */

    while (!PIR1bits.SSPIF) {
        if (PIR2bits.BCLIF) {
            return I2C_COLLISION;
        }
        if ((uint8_t)((uint8_t)g_ms - start) > I2C_TIMEOUT_MS) {
            return I2C_TIMEOUT;
        }
    }
    PIR1bits.SSPIF = 0;

    return PIR2bits.BCLIF ? I2C_COLLISION : I2C_OK;
}

static i2c_status_t send_start(void)
{
    PIR1bits.SSPIF = 0;
    SSP1CON2bits.SEN = 1;
    return wait_done();
}

static i2c_status_t send_restart(void)
{
    PIR1bits.SSPIF = 0;
    SSP1CON2bits.RSEN = 1;
    return wait_done();
}

static i2c_status_t send_byte(uint8_t value)
{
    i2c_status_t status;

    PIR1bits.SSPIF = 0;
    SSP1BUF = value;
    status = wait_done();
    if (status != I2C_OK) {
        return status;
    }
    return SSP1CON2bits.ACKSTAT ? I2C_NACK : I2C_OK;
}

i2c_status_t i2c_stop(void)
{
    PIR1bits.SSPIF = 0;
    SSP1CON2bits.PEN = 1;
    return wait_done();
}

i2c_status_t i2c_receive(uint8_t *out)
{
    i2c_status_t status;

    PIR1bits.SSPIF = 0;
    SSP1CON2bits.RCEN = 1;
    status = wait_done();
    if (status == I2C_OK) {
        *out = SSP1BUF;
    }
    return status;
}

i2c_status_t i2c_acknowledge(bool more)
{
    /* ACK asks the daughter for another byte, NACK ends the read. */
    SSP1CON2bits.ACKDT = more ? 0u : 1u;
    PIR1bits.SSPIF = 0;
    SSP1CON2bits.ACKEN = 1;
    return wait_done();
}

i2c_status_t i2c_abort(i2c_status_t reason)
{
    if (reason == I2C_NACK) {
        /* The NACK proves SDA is free, so this is not a stuck bus. A failed
         * STOP means something else is wrong, so fall through to recovery. */
        if (i2c_stop() == I2C_OK) {
            return I2C_NACK;
        }
    }
    return (i2c_bus_recover() == I2C_OK) ? reason : I2C_BUS_STUCK;
}

i2c_status_t i2c_write(uint8_t addr, const uint8_t *data, uint8_t len)
{
    i2c_status_t status = send_start();

    if (status == I2C_OK) {
        status = send_byte(ADDR_WRITE(addr));
    }
    while ((status == I2C_OK) && (len > 0u)) {
        status = send_byte(*data);
        data++;
        len--;
    }
    if (status == I2C_OK) {
        status = i2c_stop();
    }
    return (status == I2C_OK) ? I2C_OK : i2c_abort(status);
}

/* Reads len bytes (len > 0) once the read address has been ACKed. */
static i2c_status_t read_bytes(uint8_t *data, uint8_t len)
{
    i2c_status_t status = I2C_OK;

    while ((status == I2C_OK) && (len > 0u)) {
        status = i2c_receive(data);
        if (status == I2C_OK) {
            status = i2c_acknowledge(len > 1u);
        }
        data++;
        len--;
    }
    return status;
}

i2c_status_t i2c_read(uint8_t addr, uint8_t *data, uint8_t len)
{
    i2c_status_t status = send_start();

    if (status == I2C_OK) {
        status = send_byte(ADDR_READ(addr));
    }
    if (status == I2C_OK) {
        status = read_bytes(data, len);
    }
    if (status == I2C_OK) {
        status = i2c_stop();
    }
    return (status == I2C_OK) ? I2C_OK : i2c_abort(status);
}

i2c_status_t i2c_begin_read_reg(uint8_t addr, uint8_t reg)
{
    i2c_status_t status = send_start();

    if (status == I2C_OK) {
        status = send_byte(ADDR_WRITE(addr));
    }
    if (status == I2C_OK) {
        status = send_byte(reg);
    }
    if (status == I2C_OK) {
        status = send_restart();
    }
    if (status == I2C_OK) {
        status = send_byte(ADDR_READ(addr));
    }
    return (status == I2C_OK) ? I2C_OK : i2c_abort(status);
}

i2c_status_t i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t len)
{
    i2c_status_t status = i2c_begin_read_reg(addr, reg);

    if (status != I2C_OK) {
        return status;      /* i2c_begin_read_reg() already cleaned up */
    }
    status = read_bytes(data, len);
    if (status == I2C_OK) {
        status = i2c_stop();
    }
    return (status == I2C_OK) ? I2C_OK : i2c_abort(status);
}

/* Lets SCL rise, then waits for it, because a daughter may be stretching. */
static bool release_scl(void)
{
    uint8_t start = (uint8_t)g_ms;

    I2C_SCL_TRIS = 1;
    while (!I2C_SCL_PORT) {
        if ((uint8_t)((uint8_t)g_ms - start) > I2C_TIMEOUT_MS) {
            return false;
        }
    }
    return true;
}

i2c_status_t i2c_bus_recover(void)
{
    uint8_t clocks;
    bool scl_free;

    /* Take the pins back from the MSSP and bit-bang them open-drain:
     * TRIS = 1 lets the pull-up raise the line, TRIS = 0 drives it low. */
    SSP1CON1bits.SSPEN = 0;
    I2C_SDA_LAT = 0;
    I2C_SCL_LAT = 0;
    I2C_SDA_TRIS = 1;

    scl_free = release_scl();
    __delay_us(RECOVERY_HALF_US);

    for (clocks = 0u; scl_free && (clocks < BUS_RECOVERY_CLOCKS) && !I2C_SDA_PORT; clocks++) {
        I2C_SCL_TRIS = 0;
        __delay_us(RECOVERY_HALF_US);
        scl_free = release_scl();
        __delay_us(RECOVERY_HALF_US);
    }

    /* Always send a STOP. A daughter part way through sending a 1 bit looks
     * free but is still mid-transfer, and the STOP resets it. */
    if (scl_free) {
        I2C_SCL_TRIS = 0;
        __delay_us(RECOVERY_HALF_US);
        I2C_SDA_TRIS = 0;
        __delay_us(RECOVERY_HALF_US);
        scl_free = release_scl();
        __delay_us(RECOVERY_HALF_US);
        I2C_SDA_TRIS = 1;
        __delay_us(RECOVERY_HALF_US);
    }

    I2C_SDA_TRIS = 1;
    I2C_SCL_TRIS = 1;
    {
        bool bus_free = scl_free && I2C_SDA_PORT && I2C_SCL_PORT;

        i2c_init();
        return bus_free ? I2C_OK : I2C_BUS_STUCK;
    }
}
