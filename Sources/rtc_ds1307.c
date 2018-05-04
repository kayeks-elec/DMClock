/*
 * DotMatrixClock2018/rtc_ds1307.c
 *
 *  Author: kayekss
 *  Target: ATmega328P, 20.000 MHz crystal oscillator
 */

#include <stdint.h>
#include <util/twi.h>
#include "twi.h"
#include "ctime.h"
#include "rtc_ds1307.h"

// Convert a BCD value to binary
inline uint8_t bcd_to_binary(uint8_t n) {
    return 10 * (n >> 4) + (n & 0x0f);
}

// Convert a binary value to BCD
inline uint8_t binary_to_bcd(uint8_t n) {
    return ((n / 10 % 10) << 4) | (n % 10);
}

// Write clock time to RTC
// ... return 0 if no error, 1 if error in communication
uint8_t rtc_ds1307_write_clock(ctime_t* ct, dow_t dow) {
    uint8_t data[8];

    // Pack data to write
    data[0] = (0 << 7) | (binary_to_bcd(ct->s) & 0x7f);
    data[1] = binary_to_bcd(ct->m) & 0x7f;
    data[2] = (0 << 6) | (binary_to_bcd(ct->h) & 0x3f);
    data[3] = (uint8_t) dow & 0x07;
    data[4] = binary_to_bcd(ct->d) & 0x3f;
    data[5] = binary_to_bcd(ct->mo) & 0x1f;
    data[6] = (ct->yh << 4) | ct->yl;
    data[7] = (1 << 4) | (0 << 1) | (0 << 0);  // SQW enabled, 1 Hz
    // Start communication
    if (twi_start_condition()) goto error;
    if (twi_master_address(ADDRW_DS1307)) goto error;
    if (twi_master_transmit(REG_DS1307_SECONDS))
        goto error;
    for (uint8_t i = 0; i < 8; i++) {
        if (twi_master_transmit(data[i])) goto error;
    }
    twi_stop_condition();
    return 0;

error:
    twi_stop_condition();
    return 1;
}

// Read clock time from RTC
// ... return 0 if no error, 1 if error in communication
uint8_t rtc_ds1307_read_clock(ctime_t* ct, dow_t* dow) {
    uint8_t data[7];
    ctime_t ct0;
    dow_t dow0;

    // Start communication
    if (twi_start_condition()) goto error;
    if (twi_master_address(ADDRW_DS1307)) goto error;
    if (twi_master_transmit(REG_DS1307_SECONDS))
        goto error;
    if (twi_start_condition()) goto error;
    if (twi_master_address(ADDRR_DS1307)) goto error;
    for (uint8_t i = 0; i < 6; i++) {
        if (twi_master_receive(&data[i], TWI_ACK)) goto error;
    }
    if (twi_master_receive(&data[6], TWI_NACK)) goto error;
    twi_stop_condition();
    // Unpack data
    ct0.ms = 500;
    ct0.s = bcd_to_binary(data[0] & 0x7f);
    ct0.m = bcd_to_binary(data[1]);
    if (data[2] & (1 << 6)) {
        // If bit 6 is set, RTC was running at 12-hour mode
        // Then if bit 5 is set, that means hour is at PM
        ct0.h = bcd_to_binary(data[2] & 0x1f);
        if (data[2] & (1 << 5)) {
            ct0.h += 12;
        }
    } else {
        // If bit 6 is clear, RTC was running at 24-hour mode
        ct0.h = bcd_to_binary(data[2] & 0x3f);
    }
    dow0 = (dow_t) data[3];
    ct0.d = bcd_to_binary(data[4]);
    ct0.mo = bcd_to_binary(data[5]);
    ct0.yh = data[6] >> 4;
    ct0.yl = data[6] & 0x0f;
    // Copy to argument
    *ct = ct0;
    *dow = dow0;
    return 0;

error:
    twi_stop_condition();
    return 1;
}
