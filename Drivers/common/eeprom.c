/*
 * eeprom.c - See eeprom.h.
 */
#include "board.h"
#include "eeprom.h"
#include "system_tick.h"

#define EEPROM_WRITE_TIMEOUT_MS     10u     /* a write normally takes about 4 ms */

static bool wait_for_write(void)
{
    uint8_t start = (uint8_t)g_ms;

    while (EECON1bits.WR) {
        if ((uint8_t)((uint8_t)g_ms - start) > EEPROM_WRITE_TIMEOUT_MS) {
            return false;
        }
    }
    return true;
}

uint8_t eeprom_read_byte(uint8_t addr)
{
    EEADR = addr;
    EECON1bits.EEPGD = 0;   /* data EEPROM, not program memory */
    EECON1bits.CFGS = 0;
    EECON1bits.RD = 1;
    return EEDATA;
}

bool eeprom_write_byte(uint8_t addr, uint8_t value)
{
    uint8_t interrupts_on;
    bool finished;

    if (!wait_for_write()) {
        return false;
    }
    if (eeprom_read_byte(addr) == value) {
        return true;
    }

    EEADR = addr;
    EEDATA = value;
    EECON1bits.EEPGD = 0;
    EECON1bits.CFGS = 0;
    EECON1bits.WREN = 1;

    /* The hardware only accepts a write straight after this exact sequence, so
     * an interrupt must not land in the middle of it. */
    interrupts_on = INTCONbits.GIEH;
    INTCONbits.GIEH = 0;
    EECON2 = 0x55u;
    EECON2 = 0xAAu;
    EECON1bits.WR = 1;
    INTCONbits.GIEH = interrupts_on;

    finished = wait_for_write();
    EECON1bits.WREN = 0;

    return finished && (eeprom_read_byte(addr) == value);
}
