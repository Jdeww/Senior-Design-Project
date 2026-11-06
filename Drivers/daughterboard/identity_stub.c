/*
 * identity_stub.c - TEMPORARY stand-in for identity storage, for bring-up only.
 *
 * Nothing is saved, so the board forgets every motherboard at power-off and always
 * answers HELLO as new. Delete this file when the real EEPROM version is written.
 */
#include "identity.h"
#include "../common/protocol.h"
#include "../common/system_tick.h"

uint16_t identity_new_id(void)
{
    uint16_t id = (uint16_t)(tick_now() ^ 0x5A5Au);

    /* Boards that start together can pick the same ID. The motherboard catches
     * that and sends REGENERATE, by which time the tick has moved on. */
    if ((id == ID_INVALID_LOW) || (id == ID_INVALID_HIGH)) {
        id = 0x5A5Au;
    }
    return id;
}

uint16_t identity_lookup(uint16_t mb_id, uint8_t *flag)
{
    (void)mb_id;
    *flag = REPLY_FLAG_NEW;
    return identity_new_id();
}

void identity_save(uint16_t mb_id, uint16_t id)
{
    (void)mb_id;
    (void)id;
}
