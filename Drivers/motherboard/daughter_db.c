/*
 * daughter_db.c - See daughter_db.h.
 *
 * EEPROM layout:
 *   0        magic, written last so an interrupted first-time setup starts over
 *   1, 2     motherboard ID, high then low
 *   3        number of entries in use
 *   4        slot the next new entry goes in (the oldest entry, once full)
 *   5...     entries of [id_hi][id_lo][input type][last position]
 *
 * A new entry is written before the header is updated, with the count last. A
 * power cut part way through leaves the entry outside the counted table, so it
 * is simply ignored.
 */
#include "motherboard/board.h"
#include "motherboard/daughter_db.h"
#include "common/eeprom.h"
#include "common/protocol.h"
#include "common/system_tick.h"

#define ADDR_MAGIC          0u
#define ADDR_ID_HI          1u
#define ADDR_ID_LO          2u
#define ADDR_COUNT          3u
#define ADDR_NEXT           4u
#define ADDR_ENTRIES        5u

#define ENTRY_ID_HI         0u
#define ENTRY_ID_LO         1u
#define ENTRY_TYPE          2u
#define ENTRY_POSITION      3u

#define SLOT_NOT_FOUND      0xFFu

#if (ADDR_ENTRIES + (EEPROM_MAX_ENTRIES * EEPROM_ENTRY_LEN)) > 256u
    #error "daughter_db: table does not fit in the 256-byte EEPROM"
#endif

#define ID_SAMPLES          16u
#define HARDWARE_WAIT_LIMIT 0xFFFFu
#define CHS_FVR             31u     /* ADC channel: fixed voltage reference */
#define FVRS_1V             1u      /* 1.024 V */
#define ACQT_4_TAD          2u
#define ADCS_FOSC_64        6u      /* 1.33 us per bit at 48 MHz */
#define FALLBACK_ID         0x5D21u

static uint16_t s_mb_id;
static uint8_t  s_count;
static uint8_t  s_next;

static uint8_t entry_addr(uint8_t slot)
{
    return (uint8_t)(ADDR_ENTRIES + (uint8_t)(slot * EEPROM_ENTRY_LEN));
}

static bool id_is_valid(uint16_t id)
{
    return (id != ID_INVALID_LOW) && (id != ID_INVALID_HIGH);
}

/* The chip has no random number generator, so this mixes the noise in the low
 * bits of ADC readings with the free-running timers. It only runs once in the
 * life of the board, and a repeat only matters if two motherboards share the same
 * daughter boards. */
static uint16_t generate_id(void)
{
    uint16_t id = tick_now();
    uint16_t wait = HARDWARE_WAIT_LIMIT;
    uint8_t i;

    VREFCON0bits.FVRS = FVRS_1V;
    VREFCON0bits.FVREN = 1;
    while (!VREFCON0bits.FVRST && (wait > 0u)) {
        wait--;
    }

    ADCON1 = 0;                 /* reference is VDD and VSS */
    ADCON2bits.ADFM = 1;        /* right-justified, so ADRESL holds the noisy bits */
    ADCON2bits.ACQT = ACQT_4_TAD;
    ADCON2bits.ADCS = ADCS_FOSC_64;
    ADCON0bits.CHS = CHS_FVR;
    ADCON0bits.ADON = 1;

    for (i = 0u; i < ID_SAMPLES; i++) {
        wait = HARDWARE_WAIT_LIMIT;
        ADCON0bits.GO_nDONE = 1;
        while (ADCON0bits.GO_nDONE && (wait > 0u)) {
            wait--;
        }
        id = (uint16_t)((uint16_t)(id << 1) | (id >> 15));
        id ^= (uint16_t)ADRESL ^ (uint16_t)TMR2;
    }

    ADCON0bits.ADON = 0;
    VREFCON0bits.FVREN = 0;

    return id_is_valid(id) ? id : FALLBACK_ID;
}

/* Writes the header bytes in order and keeps going after a failure, so that as
 * much as possible is saved. */
static bool create_new_database(void)
{
    bool ok;

    s_mb_id = generate_id();
    s_count = 0u;
    s_next = 0u;

    ok = eeprom_write_byte(ADDR_MAGIC, 0u);
    ok = eeprom_write_byte(ADDR_ID_HI, (uint8_t)(s_mb_id >> 8)) && ok;
    ok = eeprom_write_byte(ADDR_ID_LO, (uint8_t)s_mb_id) && ok;
    ok = eeprom_write_byte(ADDR_NEXT, 0u) && ok;
    ok = eeprom_write_byte(ADDR_COUNT, 0u) && ok;
    ok = ok && eeprom_write_byte(ADDR_MAGIC, EEPROM_MAGIC);
    return ok;
}

bool db_init(void)
{
    uint16_t stored_id = (uint16_t)(((uint16_t)eeprom_read_byte(ADDR_ID_HI) << 8)
                                    | eeprom_read_byte(ADDR_ID_LO));

    if ((eeprom_read_byte(ADDR_MAGIC) != EEPROM_MAGIC) || !id_is_valid(stored_id)) {
        return create_new_database();
    }

    s_mb_id = stored_id;
    s_count = eeprom_read_byte(ADDR_COUNT);
    s_next = eeprom_read_byte(ADDR_NEXT);

    if (s_count > EEPROM_MAX_ENTRIES) {
        /* The table can't be trusted, but the motherboard's ID still can. */
        s_count = 0u;
        s_next = 0u;
        return eeprom_write_byte(ADDR_NEXT, 0u) && eeprom_write_byte(ADDR_COUNT, 0u);
    }

    /* Until the table is full, the next free slot is always the one after the last
     * entry. This also repairs a power cut between writing next and count. */
    if ((s_count < EEPROM_MAX_ENTRIES) || (s_next >= EEPROM_MAX_ENTRIES)) {
        s_next = (s_count < EEPROM_MAX_ENTRIES) ? s_count : 0u;
    }
    return true;
}

uint16_t db_mb_id(void)
{
    return s_mb_id;
}

static uint8_t find_slot(uint16_t id)
{
    uint8_t slot;

    for (slot = 0u; slot < s_count; slot++) {
        uint8_t addr = entry_addr(slot);

        if ((eeprom_read_byte((uint8_t)(addr + ENTRY_ID_HI)) == (uint8_t)(id >> 8))
                && (eeprom_read_byte((uint8_t)(addr + ENTRY_ID_LO)) == (uint8_t)id)) {
            return slot;
        }
    }
    return SLOT_NOT_FOUND;
}

bool db_contains(uint16_t id)
{
    return find_slot(id) != SLOT_NOT_FOUND;
}

bool db_save(uint16_t id, uint8_t type, uint8_t position)
{
    uint8_t slot = find_slot(id);
    uint8_t addr;
    uint8_t next;
    bool ok;

    if (slot != SLOT_NOT_FOUND) {
        addr = entry_addr(slot);
        ok = eeprom_write_byte((uint8_t)(addr + ENTRY_TYPE), type);
        return eeprom_write_byte((uint8_t)(addr + ENTRY_POSITION), position) && ok;
    }

    addr = entry_addr(s_next);
    ok = eeprom_write_byte((uint8_t)(addr + ENTRY_ID_HI), (uint8_t)(id >> 8));
    ok = ok && eeprom_write_byte((uint8_t)(addr + ENTRY_ID_LO), (uint8_t)id);
    ok = ok && eeprom_write_byte((uint8_t)(addr + ENTRY_TYPE), type);
    ok = ok && eeprom_write_byte((uint8_t)(addr + ENTRY_POSITION), position);
    if (!ok) {
        return false;
    }

    next = (uint8_t)(s_next + 1u);
    if (next >= EEPROM_MAX_ENTRIES) {
        next = 0u;
    }
    if (!eeprom_write_byte(ADDR_NEXT, next)) {
        return false;
    }
    s_next = next;

    if (s_count < EEPROM_MAX_ENTRIES) {
        if (!eeprom_write_byte(ADDR_COUNT, (uint8_t)(s_count + 1u))) {
            return false;
        }
        s_count++;
    }
    return true;
}
