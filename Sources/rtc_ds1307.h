/*
 * DotMatrixClock2018/rtc_ds1307.h
 *
 *  Author: kayekss
 *  Target: ATmega328P, 20.000 MHz crystal oscillator
 */

#ifndef RTC_DS1307_H_
#define RTC_DS1307_H_

// TWI slave address
#define ADDR7_DS1307  0x68
#define ADDRR_DS1307  ((ADDR7_DS1307 << 1) + 0x01)
#define ADDRW_DS1307  (ADDR7_DS1307 << 1)

// Enumeration table of register address
enum {
    REG_DS1307_SECONDS   = 0x00,
    REG_DS1307_MINUTES   = 0x01,
    REG_DS1307_HOURS     = 0x02,
    REG_DS1307_WEEKOFDAY = 0x03,
    REG_DS1307_DAYS      = 0x04,
    REG_DS1307_MONTHS    = 0x05,
    REG_DS1307_YEARS     = 0x06,
    REG_DS1307_CONTROL   = 0x07,
    REG_DS1307_SRAM      = 0x08
};

uint8_t rtc_ds1307_write_clock(ctime_t* ct, dow_t dow);
uint8_t rtc_ds1307_read_clock(ctime_t* ct, dow_t* dow);

#endif
