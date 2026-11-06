# i2c_master.h

Source: `Drivers/motherboard/i2c_master.h`
Implementation: [i2c_master.c](i2c_master.c.md)

## What it does

Defines the public interface for using the I2C bus from the motherboard: complete read and write transactions, smaller steps for variable-length reads, and bus recovery. Call these functions from the main loop only, never from an interrupt.

## Inputs and outputs

### Status codes (`i2c_status_t`)

Almost every function returns one of these:

| Status | Meaning |
|---|---|
| `I2C_OK` | Worked |
| `I2C_NACK` | Nothing acknowledged. This is normal for HELLO at the end of the chain. |
| `I2C_TIMEOUT` | The hardware did not finish in time |
| `I2C_COLLISION` | The bus did not match what the master was driving |
| `I2C_BUS_STUCK` | Recovery failed; a line is still held low |
| `I2C_BAD_DATA` | The transfer worked but the contents failed a check. Set by `daughter_protocol`, not by the functions in this file. |

### Setup and recovery

| Function | Inputs | Output |
|---|---|---|
| `i2c_init()` | None | None. Sets up the pins and MSSP for 100 kHz. |
| `i2c_bus_recover()` | None | `I2C_OK` or `I2C_BUS_STUCK` |

### Complete transactions

| Function | Inputs | Output |
|---|---|---|
| `i2c_write(addr, data, len)` | 7-bit address; bytes to send and how many | Status |
| `i2c_read(addr, data, len)` | 7-bit address; buffer and byte count (must be above 0) | Status; `data` is filled |
| `i2c_read_reg(addr, reg, data, len)` | 7-bit address; register; buffer and byte count (must be above 0) | Status; `data` is filled |

### Steps of a register read

| Function | Inputs | Output |
|---|---|---|
| `i2c_begin_read_reg(addr, reg)` | 7-bit address; register | Status. Cleans up after itself on failure. |
| `i2c_receive(out)` | Where to store the byte | Status; `*out` is set |
| `i2c_acknowledge(more)` | `true` to ask for another byte, `false` to end the read | Status |
| `i2c_stop()` | None | Status |
| `i2c_abort(reason)` | The status that caused the failure | Final status, after cleanup |

## How it works

### Error cleanup

The complete transactions clean up after themselves, so the caller only has to check the status:

| Failure | Cleanup | Returned status |
|---|---|---|
| `I2C_NACK` | Sends STOP | `I2C_NACK` |
| Timeout or collision | Runs bus recovery | The original error, or `I2C_BUS_STUCK` if recovery failed |

### Reading a length that isn't known in advance

The steps exist for reads where one byte decides how many follow, such as the event stream. Receiving and acknowledging are separate so the caller can look at a byte before choosing the response:

```c
status = i2c_begin_read_reg(addr, reg);   /* cleans up if it fails */
if (status != I2C_OK) return status;

status = i2c_receive(&first);             /* look at the byte...  */
if (status == I2C_OK) {
    status = i2c_acknowledge(first > 0);  /* ...then respond      */
}
/* read any further bytes the same way */

if (status == I2C_OK) status = i2c_stop();
if (status != I2C_OK) return i2c_abort(status);
```

Unlike `i2c_begin_read_reg()`, the other steps don't clean up on failure, so the caller must pass any error to `i2c_abort()`.
