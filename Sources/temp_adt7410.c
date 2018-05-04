/*
 * DotMatrixClock2018/temp_adt7410.c
 *
 *  Author: kayekss
 *  Target: ATmega328P, 20.000 MHz crystal oscillator
 */

#include <stdbool.h>
#include <stdint.h>
#include <util/twi.h>
#include "twi.h"
#include "temp_adt7410.h"

// Send configuration to ADT7410
// ... return 0 if no error, 1 if error in communication
uint8_t temp_adt7410_set_config(uint8_t config) {
    // Start communication
    if (twi_start_condition()) goto error;
    if (twi_master_address(ADDRW_ADT7410)) goto error;
    if (twi_master_transmit(REG_ADT7410_CONFIG))
        goto error;
    twi_master_transmit(config);
    twi_stop_condition();
    return 0;

error:
    twi_stop_condition();
    return 1;
}

// Read temperature sensor value
// ... return 0 if no error, 1 if error in communication
uint8_t temp_adt7410_read_temperature(temp_adt7410_t* value, uint8_t* flags,
    config_ad7410resol_t resol) {
    uint8_t datah, datal;
    uint32_t temp_x10k = 0;
    uint8_t flags0 = 0x00;
    temp_adt7410_t value0;

    // Start communication
    if (twi_start_condition()) goto error;
    if (twi_master_address(ADDRW_ADT7410)) goto error;
    if (twi_master_transmit(REG_ADT7410_VALUE_MSB))
        goto error;
    if (twi_start_condition()) goto error;
    if (twi_master_address(ADDRR_ADT7410)) goto error;
    if (twi_master_receive(&datah, TWI_ACK)) goto error;
    if (twi_master_receive(&datal, TWI_NACK)) goto error;
    twi_stop_condition();
    // Unpack data
    if (resol == CONFIG_ADT7410_RESOL_13BITS) {
        flags0 = datal & 0x07;
        datal &= ~0x07;
    }
    if (datah & 0x80) {
        value0.sign = 1;
        temp_x10k = (65536ul - ((uint32_t) (datah << 8) | datal)) * 10000 >> 7;
    } else {
        value0.sign = 0;
        temp_x10k = ((uint32_t) (datah << 8) | datal) * 10000 >> 7;
    }
    value0.integer = temp_x10k / 10000;
    value0.fraction_x10k = temp_x10k % 10000;
    // Copy to argument
    *value = value0;
    *flags = flags0;
    return 0;

error:
    twi_stop_condition();
    return 1;
}
