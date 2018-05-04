/*
 * ctime.h
 *
 *  Author: kayekss
 *  Target: unspecified
 */

#ifndef CTIME_H_
#define CTIME_H_

// Internal time structure
typedef struct {
    // High and low digits of years (0..9)
    uint8_t yh, yl;
    // Months (1..12)
    uint8_t mo;
    // Days (1..31)
    uint8_t d;
    // Hours (0..23)
    uint8_t h;
    // Minutes (0..59)
    uint8_t m;
    // Seconds (0..59)
    uint8_t s;
    // Milliseconds (0..999)
    uint16_t ms;
} ctime_t;

// Enumerated labels for week
typedef enum {
    DOW_SUNDAY    = 0x01,
    DOW_MONDAY    = 0x02,
    DOW_TUESDAY   = 0x03,
    DOW_WEDNESDAY = 0x04,
    DOW_THURSDAY  = 0x05,
    DOW_FRIDAY    = 0x06,
    DOW_SATURDAY  = 0x07
} dow_t;

uint8_t is_leap_year(ctime_t* ct);
uint8_t days_in_month(ctime_t* ct);
uint8_t ctime_increment_tick(ctime_t* ct);
uint8_t ctime_increment_day(ctime_t* ct);
uint8_t ctime_decrement_day(ctime_t* ct);
uint8_t ctime_check_error(ctime_t* ct);
dow_t dayofweek(ctime_t* ct);

#endif
