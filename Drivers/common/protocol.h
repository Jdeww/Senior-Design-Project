/*
 * protocol.h - Constants shared by the motherboard and daughter board firmware.
 *
 * Both firmwares include this file, so every byte value, address and timing
 * that the two sides must agree on lives here and nowhere else.
 *
 * Source: "Macro Pad Firmware Driver Summary" (Oct 30, 2026).
 */
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

/* ------------------------------------------------------------------------ */
/* Firmware                                                                 */
/* ------------------------------------------------------------------------ */
#define FW_VERSION                  0x01u

/* ------------------------------------------------------------------------ */
/* I2C bus                                                                  */
/* ------------------------------------------------------------------------ */
/* OPEN QUESTION: the Driver Summary says 100 kHz, the Firmware Summary says
 * 400 kHz. Start at 100 kHz for bring-up (more forgiving of pogo pins and
 * long chains), then try 400 kHz once the bus is proven. */
#define I2C_BUS_HZ                  100000UL

#define I2C_ADDR_GENERAL_CALL       0x00u   /* RESET_CHAIN is sent here      */
#define I2C_ADDR_DISCOVERY          0x2Fu   /* unregistered daughters        */
#define I2C_ADDR_POS_BASE           0x30u   /* registered: 0x30 + position   */

#define CHAIN_MAX_POSITIONS         4u      /* positions 1..4 = 0x31..0x34   */
#define POSITION_MOTHERBOARD        0u      /* reserved for the main board   */

#define I2C_ADDR_FOR_POS(pos)       ((uint8_t)(I2C_ADDR_POS_BASE + (pos)))

/* ------------------------------------------------------------------------ */
/* Handshake messages (motherboard -> daughter)                             */
/* ------------------------------------------------------------------------ */
#define MSG_RESET_CHAIN             0xA4u   /* [A4]               to 0x00    */
#define MSG_HELLO                   0xB0u   /* [B0][mb_hi][mb_lo] to 0x2F    */
#define MSG_ACCEPT                  0xA1u   /* [A1][id_hi][id_lo][position]  */
#define MSG_REGENERATE              0xA2u   /* [A2][id_hi][id_lo]            */

#define MSG_RESET_CHAIN_LEN         1u
#define MSG_HELLO_LEN               3u
#define MSG_ACCEPT_LEN              4u
#define MSG_REGENERATE_LEN          3u

/* ------------------------------------------------------------------------ */
/* HELLO reply (daughter -> motherboard, 8 bytes read from 0x2F)            */
/*   [flag][id_hi][id_lo][type][~flag][~id_hi][~id_lo][~type]               */
/* ------------------------------------------------------------------------ */
#define REPLY_LEN                   8u

#define REPLY_FLAG_SEEN             0u      /* joined this motherboard before */
#define REPLY_FLAG_NEW              1u      /* fresh random ID                */

#define REPLY_IDX_FLAG              0u
#define REPLY_IDX_ID_HI             1u
#define REPLY_IDX_ID_LO             2u
#define REPLY_IDX_TYPE              3u
#define REPLY_IDX_COMPLEMENT        4u      /* bytes 4..7 = ~bytes 0..3      */

/* IDs a daughter must never use (random ID generator rejects these). */
#define ID_INVALID_LOW              0x0000u
#define ID_INVALID_HIGH             0xFFFFu

/* ------------------------------------------------------------------------ */
/* Daughter register map (at 0x30 + position)                               */
/* ------------------------------------------------------------------------ */
#define REG_INPUT_TYPE              0x00u   /* 1..4, 0 = nothing fitted      */
#define REG_INPUT_COUNT             0x01u
#define REG_FW_VERSION              0x02u
#define REG_ID_HI                   0x03u
#define REG_ID_LO                   0x04u
#define REG_EVENTS                  0x05u   /* [count] then [index][value]...*/
#define REG_POSITION                0x06u
#define REG_SNAPSHOT_BASE           0x20u   /* 0x20 + n = current value of n */

#define EVENT_FIFO_DEPTH            16u     /* entries of [index][value]     */

/* Reading REG_EVENTS: the master ACKs every value byte, which is what removes
 * that event from the FIFO. If any events were read, it then reads one extra
 * byte and NACKs it to end the read. */

/* ------------------------------------------------------------------------ */
/* Input types                                                              */
/* ------------------------------------------------------------------------ */
#define INPUT_TYPE_NONE             0u
#define INPUT_TYPE_KEYS             1u
#define INPUT_TYPE_ENCODER          2u
#define INPUT_TYPE_SLIDER           3u
#define INPUT_TYPE_COCKPIT          4u

#define INPUT_COUNT_KEYS            8u
#define INPUT_COUNT_ENCODER         2u      /* 0 = detent count, 1 = button  */
#define INPUT_COUNT_SLIDER          1u
/* Cockpit count = number of switches fitted (OPEN QUESTION, per board).    */

#define ENCODER_IDX_DETENTS         0u      /* signed 8-bit detents          */
#define ENCODER_IDX_BUTTON          1u

/* ID resistor -> 8-bit ADC windows (10k pull-up to 3.3 V, resistor to GND) */
#define ADC_WIN_KEYS_MIN            22u
#define ADC_WIN_KEYS_MAX            64u     /* 2.0 kOhm                      */
#define ADC_WIN_ENCODER_MIN         65u
#define ADC_WIN_ENCODER_MAX         106u    /* 4.99 kOhm                     */
#define ADC_WIN_SLIDER_MIN          107u
#define ADC_WIN_SLIDER_MAX          149u    /* 10 kOhm                       */
#define ADC_WIN_COCKPIT_MIN         150u
#define ADC_WIN_COCKPIT_MAX         191u    /* 20 kOhm                       */
#define ADC_WIN_SPARE_MIN           192u
#define ADC_WIN_SPARE_MAX           233u    /* 49.9 kOhm, not yet assigned   */
#define ADC_WIN_NONE_MIN            234u    /* open: nothing fitted          */

/* ------------------------------------------------------------------------ */
/* USB reports (device -> PC, 8 bytes, unused bytes 0)                      */
/* ------------------------------------------------------------------------ */
#define USB_REPORT_LEN              8u

#define RPT_INPUT_EVENT             0x01u   /* [01][pos][index][value]       */
#define RPT_ATTACH                  0x02u   /* [02][pos][id_hi][id_lo][type][count] */
#define RPT_DETACH                  0x03u   /* [03][pos][id_hi][id_lo]       */
#define RPT_CHAIN_RESET             0x04u   /* [04]                          */
#define RPT_CHAIN_COMPLETE          0x05u   /* [05][board count]             */

/* PC -> device (OUT) commands */
#define CMD_LIST_MODULES            0x10u
#define CMD_FW_VERSION              0x11u

#define USB_HID_USAGE_PAGE          0xFF00u /* vendor-defined                */

/* ------------------------------------------------------------------------ */
/* EEPROM                                                                   */
/* ------------------------------------------------------------------------ */
#define EEPROM_MAGIC                0xA5u
#define EEPROM_ENTRY_LEN            4u
#define EEPROM_MAX_ENTRIES          60u     /* oldest replaced when full     */

/* ------------------------------------------------------------------------ */
/* Timing (ms unless noted)                                                 */
/* ------------------------------------------------------------------------ */
#define TICK_MS                     1u      /* Timer2 system tick            */

/* Motherboard */
#define POLL_INTERVAL_MS            5u      /* read events from each position*/
#define HELLO_INTERVAL_MS           100u    /* look for a board at chain end */
#define I2C_TIMEOUT_MS              2u      /* every MSSP wait               */
#define GATE_SETTLE_MS              1u      /* after ACCEPT                  */
#define NACK_LIMIT                  2u      /* NACKs in a row -> re-enumerate*/
#define REPLY_RETRIES               3u      /* bad complement -> re-read     */
#define BUS_RECOVERY_CLOCKS         9u

/* Daughter */
#define KEY_DEBOUNCE_SCANS          5u      /* 5 matching 1 ms scans         */
#define COCKPIT_DEBOUNCE_MS         20u
#define SLIDER_SAMPLE_MS            2u
#define SLIDER_HYSTERESIS           2u      /* +/- counts                    */
#define ENCODER_COUNTS_PER_DETENT   4u      /* OPEN QUESTION: check encoder  */

#endif /* PROTOCOL_H */
