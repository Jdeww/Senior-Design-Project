/*
 * i2c_slave.c - See i2c_slave.h.
 *
 * Timing rules that shape this file:
 *
 * - RESET_CHAIN and ACCEPT take effect at the STOP, not when their bytes arrive.
 *   Moving the gate earlier would cut boards further down the chain off in the
 *   middle of a transaction, before they have seen the STOP.
 *
 * - The EEPROM lookup for HELLO is too slow for an interrupt, so it runs in
 *   i2c_slave_task(). If the motherboard reads the reply before it is ready, the
 *   clock is held low until it is. The master's timeout tolerates this.
 */
#include "daughterboard/board.h"
#include "daughterboard/i2c_slave.h"
#include "daughterboard/identity.h"
#include "common/protocol.h"

#define SSPCON1_SLAVE       0x36u   /* SSPEN = 1, CKP = 1, SSPM = 0110 (7-bit)  */
#define SSPSTAT_100KHZ      0x80u   /* SMP = 1: slew control off below 400 kHz  */
#define SSP_ADDR(a)         ((uint8_t)((a) << 1))

#define RX_MAX              MSG_ACCEPT_LEN  /* the longest incoming message     */
#define FIFO_MASK           (EVENT_FIFO_DEPTH - 1u)
#define READ_PAD            0xFFu   /* fails the master's complement check      */

typedef enum { ST_WAITING, ST_REPLYING, ST_ACTIVE } slave_state_t;
typedef enum { JOB_NONE, JOB_HELLO, JOB_REGENERATE } slave_job_t;

/* Handshake state, shared between the interrupt and the main loop. */
static volatile uint8_t s_state;
static volatile uint8_t s_job;
static volatile bool    s_reply_ready;
static volatile bool    s_clock_held;   /* the master is waiting for the reply */
static volatile bool    s_save_pending;
static volatile uint8_t s_position;
static volatile uint16_t s_mb_id;
static volatile uint16_t s_my_id;
static volatile uint8_t s_reply_flag;
static volatile uint8_t s_reply[REPLY_LEN];

/* Current transaction, used only by the interrupt. */
static uint8_t s_rx[RX_MAX];
static uint8_t s_rx_len;
static bool    s_rx_general_call;
static uint8_t s_reply_idx;
static uint8_t s_reg;
static bool    s_in_stream;
static bool    s_stream_on_value;
static bool    s_stream_value_sent;
static uint8_t s_stream_left;

/* Fixed at init. */
static uint8_t s_input_type;
static uint8_t s_input_count;

static volatile uint8_t s_snapshot[I2C_SLAVE_SNAPSHOT_COUNT];

/* Event FIFO. The main loop only writes head and the interrupt only writes
 * tail. Both run freely and wrap, so head - tail is always the count. */
static volatile uint8_t s_fifo_index[EVENT_FIFO_DEPTH];
static volatile uint8_t s_fifo_value[EVENT_FIFO_DEPTH];
static volatile uint8_t s_fifo_head;
static volatile uint8_t s_fifo_tail;

void i2c_slave_init(uint8_t input_type, uint8_t input_count)
{
    s_input_type = input_type;
    s_input_count = input_count;
    s_state = ST_WAITING;

    /* The hardware pull-down already holds the gate open; keep it that way. */
    GATE_ANS = 0;
    GATE_LAT = 0;
    GATE_TRIS = 0;

    I2C_SDA_ANS = 0;
    I2C_SCL_ANS = 0;
    I2C_SDA_TRIS = 1;
    I2C_SCL_TRIS = 1;

    SSP1ADD = SSP_ADDR(I2C_ADDR_DISCOVERY);
    SSP1MSK = 0xFFu;
    SSP1STAT = SSPSTAT_100KHZ;
    SSP1CON2 = 0;
    SSP1CON2bits.GCEN = 1;      /* hear RESET_CHAIN                          */
    SSP1CON2bits.SEN = 1;       /* hold the clock after every byte           */
    SSP1CON3 = 0;
    SSP1CON3bits.PCIE = 1;      /* interrupt on STOP                         */
    SSP1CON1 = SSPCON1_SLAVE;

    PIR1bits.SSPIF = 0;
    IPR1bits.SSPIP = 1;         /* high priority                             */
    PIE1bits.SSPIE = 1;
}

/* ---------------------------------------------------------------- reading */

static uint8_t read_register(uint8_t reg)
{
    if ((reg >= REG_SNAPSHOT_BASE) && (reg < (REG_SNAPSHOT_BASE + I2C_SLAVE_SNAPSHOT_COUNT))) {
        return s_snapshot[reg - REG_SNAPSHOT_BASE];
    }
    switch (reg) {
        case REG_INPUT_TYPE:  return s_input_type;
        case REG_INPUT_COUNT: return s_input_count;
        case REG_FW_VERSION:  return FW_VERSION;
        case REG_ID_HI:       return (uint8_t)(s_my_id >> 8);
        case REG_ID_LO:       return (uint8_t)s_my_id;
        case REG_EVENTS:      return (uint8_t)(s_fifo_head - s_fifo_tail);
        case REG_POSITION:    return s_position;
        default:              return READ_PAD;
    }
}

/* Called only after the master ACKs the previous byte. An event leaves the FIFO
 * once its value byte has been ACKed, so a read that fails at any point leaves
 * the unconfirmed event queued to be sent again on the next poll. */
static uint8_t next_stream_byte(void)
{
    uint8_t slot;

    if (s_stream_value_sent) {
        s_stream_value_sent = false;
        s_fifo_tail = (uint8_t)(s_fifo_tail + 1u);
        s_stream_left--;
    }
    if (s_stream_left == 0u) {
        return READ_PAD;
    }
    slot = s_fifo_tail & FIFO_MASK;
    if (!s_stream_on_value) {
        s_stream_on_value = true;
        return s_fifo_index[slot];
    }
    s_stream_on_value = false;
    s_stream_value_sent = true;
    return s_fifo_value[slot];
}

static uint8_t next_read_byte(void)
{
    uint8_t out;

    if (s_state != ST_ACTIVE) {
        if (s_reply_ready && (s_reply_idx < REPLY_LEN)) {
            out = s_reply[s_reply_idx];
            s_reply_idx++;
            return out;
        }
        return READ_PAD;
    }
    if (s_in_stream) {
        return next_stream_byte();
    }
    out = read_register(s_reg);
    s_reg++;
    return out;
}

/* Called on a read address match. Clears the read position, so the master can
 * re-read the whole reply after a bad complement. */
static void begin_read(void)
{
    s_rx_len = 0u;
    s_reply_idx = 0u;
    s_in_stream = false;

    if (s_state == ST_ACTIVE) {
        if (s_reg == REG_EVENTS) {
            s_in_stream = true;
            s_stream_on_value = false;
            s_stream_value_sent = false;
            s_stream_left = (uint8_t)(s_fifo_head - s_fifo_tail);
            SSP1BUF = s_stream_left;
            SSP1CON1bits.CKP = 1;
            return;
        }
    } else if ((s_state == ST_REPLYING) && !s_reply_ready) {
        s_clock_held = true;    /* i2c_slave_task() loads the byte and releases */
        return;
    }
    SSP1BUF = next_read_byte();
    SSP1CON1bits.CKP = 1;
}

/* ---------------------------------------------------------------- messages */

static void reset_chain(void)
{
    GATE_LAT = 0;
    SSP1ADD = SSP_ADDR(I2C_ADDR_DISCOVERY);
    s_state = ST_WAITING;
    s_job = JOB_NONE;
    s_reply_ready = false;
    s_position = 0u;
    s_reg = 0u;
}

static bool message_names_my_id(void)
{
    uint16_t id = (uint16_t)(((uint16_t)s_rx[1] << 8) | s_rx[2]);

    return s_reply_ready && (id == s_my_id);
}

/* Called on STOP. Bytes received at a position address are just the register
 * pointer, so only general calls and discovery messages get here with a meaning. */
static void handle_message(void)
{
    uint8_t len = s_rx_len;

    s_rx_len = 0u;
    if (len == 0u) {
        return;
    }
    if (s_rx_general_call) {
        if ((len == MSG_RESET_CHAIN_LEN) && (s_rx[0] == MSG_RESET_CHAIN)) {
            reset_chain();
        }
        return;
    }
    if (s_state == ST_ACTIVE) {
        return;
    }

    if ((len == MSG_HELLO_LEN) && (s_rx[0] == MSG_HELLO)) {
        s_mb_id = (uint16_t)(((uint16_t)s_rx[1] << 8) | s_rx[2]);
        s_reply_ready = false;
        s_job = JOB_HELLO;
        s_state = ST_REPLYING;
    } else if ((len == MSG_REGENERATE_LEN) && (s_rx[0] == MSG_REGENERATE)) {
        if ((s_state == ST_REPLYING) && message_names_my_id()) {
            s_reply_ready = false;
            s_job = JOB_REGENERATE;
        }
    } else if ((len == MSG_ACCEPT_LEN) && (s_rx[0] == MSG_ACCEPT)) {
        if ((s_state == ST_REPLYING) && message_names_my_id()
                && (s_rx[3] >= 1u) && (s_rx[3] <= CHAIN_MAX_POSITIONS)) {
            s_position = s_rx[3];
            SSP1ADD = SSP_ADDR(I2C_ADDR_FOR_POS(s_position));
            s_reg = 0u;
            s_save_pending = (s_reply_flag == REPLY_FLAG_NEW);
            s_state = ST_ACTIVE;
            GATE_LAT = 1;
        }
    }
}

static void receive_byte(void)
{
    uint8_t value = SSP1BUF;

    /* At a position address, the first byte selects the register. It must take
     * effect now, because a RESTART follows and there will be no STOP first. */
    if ((s_state == ST_ACTIVE) && !s_rx_general_call) {
        if (s_rx_len == 0u) {
            s_reg = value;
            s_rx_len = 1u;
        }
        return;
    }
    if (s_rx_len < RX_MAX) {
        s_rx[s_rx_len] = value;
    }
    /* A message that is too long ends up with a length no message has. */
    if (s_rx_len <= RX_MAX) {
        s_rx_len++;
    }
}

/* ---------------------------------------------------------------- interrupt */

void i2c_slave_isr(void)
{
    uint8_t address;

    PIR1bits.SSPIF = 0;

    if (SSP1CON1bits.SSPOV || SSP1CON1bits.WCOL) {
        (void)SSP1BUF;
        SSP1CON1bits.SSPOV = 0;
        SSP1CON1bits.WCOL = 0;
        SSP1CON1bits.CKP = 1;
        return;
    }

    if (SSP1STATbits.P) {
        handle_message();
        return;
    }

    if (!SSP1STATbits.D_nA) {
        address = SSP1BUF;      /* reading it clears BF */
        if (SSP1STATbits.R_nW) {
            begin_read();
        } else {
            s_rx_len = 0u;
            s_rx_general_call = (address == SSP_ADDR(I2C_ADDR_GENERAL_CALL));
            SSP1CON1bits.CKP = 1;
        }
    } else if (SSP1STATbits.R_nW) {
        /* ACKSTAT set means the master NACKed: the read is over, send nothing. */
        if (!SSP1CON2bits.ACKSTAT) {
            SSP1BUF = next_read_byte();
        }
        SSP1CON1bits.CKP = 1;
    } else if (SSP1STATbits.BF) {
        receive_byte();
        SSP1CON1bits.CKP = 1;
    } else {
        /* A master NACK can also land here: R_nW is only valid up to a NACK,
         * and no byte was received. Nothing to do but release the clock. */
        SSP1CON1bits.CKP = 1;
    }
}

/* ---------------------------------------------------------------- main loop */

static void publish_reply(uint16_t id, uint8_t flag)
{
    uint8_t i;

    s_my_id = id;
    s_reply_flag = flag;
    s_reply[REPLY_IDX_FLAG] = flag;
    s_reply[REPLY_IDX_ID_HI] = (uint8_t)(id >> 8);
    s_reply[REPLY_IDX_ID_LO] = (uint8_t)id;
    s_reply[REPLY_IDX_TYPE] = s_input_type;
    for (i = 0u; i < REPLY_IDX_COMPLEMENT; i++) {
        s_reply[REPLY_IDX_COMPLEMENT + i] = (uint8_t)~s_reply[i];
    }
    s_reply_ready = true;

    if (s_clock_held) {
        s_clock_held = false;
        s_reply_idx = 1u;
        SSP1BUF = s_reply[0];
        SSP1CON1bits.CKP = 1;
    }
}

void i2c_slave_task(void)
{
    uint8_t job;
    uint8_t flag = REPLY_FLAG_NEW;
    bool save;
    uint16_t mb_id;
    uint16_t id;

    PIE1bits.SSPIE = 0;
    job = s_job;
    s_job = JOB_NONE;
    save = s_save_pending;
    s_save_pending = false;
    mb_id = s_mb_id;
    id = s_my_id;
    PIE1bits.SSPIE = 1;

    /* Save first, so a HELLO straight after a reset finds this pair. */
    if (save) {
        identity_save(mb_id, id);
    }
    if (job == JOB_NONE) {
        return;
    }

    if (job == JOB_HELLO) {
        id = identity_lookup(mb_id, &flag);
    } else {
        id = identity_new_id();
    }

    /* While that ran, a RESET_CHAIN or a new HELLO may have arrived. If so, this
     * answer is out of date and must not be published. */
    PIE1bits.SSPIE = 0;
    if ((s_job == JOB_NONE) && (s_state == ST_REPLYING)) {
        publish_reply(id, flag);
    }
    PIE1bits.SSPIE = 1;
}

bool i2c_slave_push_event(uint8_t index, uint8_t value)
{
    uint8_t head = s_fifo_head;
    uint8_t slot;

    if ((uint8_t)(head - s_fifo_tail) >= EVENT_FIFO_DEPTH) {
        return false;
    }
    slot = head & FIFO_MASK;
    s_fifo_index[slot] = index;
    s_fifo_value[slot] = value;
    s_fifo_head = (uint8_t)(head + 1u);     /* publish only after the data */
    return true;
}

void i2c_slave_set_snapshot(uint8_t input, uint8_t value)
{
    if (input < I2C_SLAVE_SNAPSHOT_COUNT) {
        s_snapshot[input] = value;
    }
}
