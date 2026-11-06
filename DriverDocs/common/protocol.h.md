# protocol.h

Source: `Drivers/common/protocol.h`

## What it does

Holds every constant the motherboard and daughter board firmware must agree on: addresses, message bytes, register numbers, timings and data formats. Both firmwares include this one file, so the two sides can't drift apart.

It contains only `#define`s and one helper macro. It has no functions or variables, and it uses no memory unless a value is used by other code.

## Inputs and outputs

There are no runtime inputs or outputs. The file's only input is its own include, `<stdint.h>`, and its output is the set of constants below.

| Group | Key values | Used by the current code? |
|---|---|---|
| Firmware | `FW_VERSION` = 0x01 | Yes, in the daughter's firmware version register |
| I2C bus | `I2C_BUS_HZ` = 100 kHz; addresses 0x00, 0x2F and 0x30 + position; `CHAIN_MAX_POSITIONS` = 4 | Yes |
| Handshake messages | `MSG_RESET_CHAIN` 0xA4, `MSG_HELLO` 0xB0, `MSG_ACCEPT` 0xA1, `MSG_REGENERATE` 0xA2, and their lengths | Yes |
| HELLO reply | 8 bytes; flag values and byte positions | Yes |
| Daughter registers | `REG_INPUT_TYPE` 0x00 through `REG_POSITION` 0x06, `REG_SNAPSHOT_BASE` 0x20, `EVENT_FIFO_DEPTH` 16 | Yes |
| Timing | `TICK_MS`, `I2C_TIMEOUT_MS`, `REPLY_RETRIES`, `BUS_RECOVERY_CLOCKS` | Yes |
| Timing | `POLL_INTERVAL_MS`, `HELLO_INTERVAL_MS`, `GATE_SETTLE_MS`, `NACK_LIMIT`, debounce and slider values | Not yet |
| Input types | `INPUT_TYPE_*`, `INPUT_COUNT_*`, `ADC_WIN_*` | Not yet |
| USB reports | `RPT_*`, `CMD_*`, `USB_REPORT_LEN`, `USB_HID_USAGE_PAGE` | Not yet |
| EEPROM | `EEPROM_MAGIC`, `EEPROM_ENTRY_LEN`, `EEPROM_MAX_ENTRIES` | Not yet |
| ID limits | `ID_INVALID_LOW`, `ID_INVALID_HIGH` | Not yet |

## How it works

### Addressing

The chain uses three kinds of address:

| Address | Who answers |
|---|---|
| `0x00` (general call) | Every daughter; used for `RESET_CHAIN` |
| `0x2F` (discovery) | The one board that has not been registered yet |
| `0x31`..`0x34` | Registered boards, from `I2C_ADDR_FOR_POS(pos)` = `0x30 + pos` |

Position 0 (`POSITION_MOTHERBOARD`) is reserved for the motherboard.

### Message formats

| Message | Sent to | Bytes |
|---|---|---|
| RESET_CHAIN | 0x00 | `[A4]` |
| HELLO | 0x2F | `[B0][mb_hi][mb_lo]` |
| ACCEPT | 0x2F | `[A1][id_hi][id_lo][position]` |
| REGENERATE | 0x2F | `[A2][id_hi][id_lo]` |
| Reply (read) | 0x2F | `[flag][id_hi][id_lo][type][~flag][~id_hi][~id_lo][~type]` |

The last four reply bytes are the bitwise inverse of the first four, which lets the motherboard detect a corrupted reply. For example, a seen board with ID 0x3AC7 and key switches replies `00 3A C7 01 FF C5 38 FE`.

### Open items in the file

- **Bus speed:** a comment records that the two summary documents disagree on 100 kHz and 400 kHz. The file uses 100 kHz.
- **Cockpit switch count:** the number of switches per board is undecided.
- **Encoder resolution:** `ENCODER_COUNTS_PER_DETENT` = 4 still needs checking against the actual part.
