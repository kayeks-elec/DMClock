/*
 * ctime.c
 *
 *  Author: kayekss
 *  Target: unspecified
 */

#include <stdint.h>
#include <stdbool.h>
#include "ctime.h"

// Check if the time is in a leap year, return 1 when it is
uint8_t is_leap_year(ctime_t* ct) {
    uint16_t y = 2000 + 10 * ct->yh + ct->yl;
    
    return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0 ? 1 : 0;
}

// Get numbers of days in month in the time
uint8_t days_in_month(ctime_t* ct) {
    uint8_t const DAYS_IN_MONTH_TABLE[12] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    uint8_t days = 0;

    if (ct->mo == 2) {
        // Leap year check if it is February
        days = 28 + is_leap_year(ct);
    } else if (ct->mo == 0 || ct->mo >= 12) {
        days = 0;
    } else {
        days = DAYS_IN_MONTH_TABLE[ct->mo - 1];
    }
    return days;
}

// Increment clock time by one tick, return
// ... bit<7:4>  always zero
// ...    <3>    one if carry occurs in hours
// ...    <2>    one if carry occurs in minutes
// ...    <1>    one if carry occurs in seconds
// ...    <0>    one if carry occurs in sub-seconds
inline uint8_t ctime_increment_tick(ctime_t* ct) {
    uint8_t result = 0;
    
    if (ct->ms >= 999) {
        result |= (1 << 0);
        ct->ms = 0;
        if (ct->s >= 59) {
            result |= (1 << 1);
            ct->s = 0;
            if (ct->m >= 59) {
                result |= (1 << 2);
                ct->m = 0;
                if (ct->h >= 23) {
                    result |= (1 << 3);
                    ct->h = 0;
                    result |= ctime_increment_day(ct);
                } else {
                    ct->h++;
                }
            } else {
                ct->m++;
            }
        } else {
            ct->s++;
        }
    } else {
        ct->ms++;
    }
    return result;
}

// Increment clock time by one day; return
// ... bit<7>    one if carry occurs in high digit of years
// ...    <6>    one if carry occurs in low digit of years
// ...    <5>    one if carry occurs in months
// ...    <4>    one if carry occurs in days
// ...    <3:0>  always zero
inline uint8_t ctime_increment_day(ctime_t* ct) {
    uint8_t result = 0;
    
    if (ct->d >= days_in_month(ct)) {
        result |= (1 << 4);
        ct->d = 1;
        if (ct->mo >= 12) {
            result |= (1 << 5);
            ct->mo = 1;
            if (ct->yl >= 9) {
                result |= (1 << 6);
                ct->yl = 0;
                if (ct->yh >= 9) {
                    result |= (1 << 7);
                    ct->yh = 0;
                } else {
                    ct->yh++;
                }
            } else {
                ct->yl++;
            }
        } else {
            ct->mo++;
        }
    } else {
        ct->d++;
    }
    return result;
}

// Decrement clock time by one day; return
// ... bit<7>    one if borrow occurs in high digit of years
// ...    <6>    one if borrow occurs in low digit of years
// ...    <5>    one if borrow occurs in months
// ...    <4>    one if borrow occurs in days
// ...    <3:0>  always zero
inline uint8_t ctime_decrement_day(ctime_t* ct) {
    uint8_t result = 0;
    
    if (ct->d == 1) {
        result |= (1 << 4);
        if (ct->mo == 1) {
            result |= (1 << 5);
            ct->mo = 12;
            if (ct->yl == 0) {
                result |= (1 << 6);
                ct->yl = 9;
                if (ct->yh == 0) {
                    result |= (1 << 7);
                    ct->yh = 9;
                } else {
                    ct->yh--;
                }
            } else {
                ct->yl--;
            }
        } else {
            ct->mo--;
        }
        ct->d = days_in_month(ct);
    } else {
        ct->d--;
    }
    return result;
}

// Check if the time has valid values in structure,
// ... return zero if it is, non-zero if something is wrong
uint8_t ctime_check_error(ctime_t* ct) {
    uint8_t result = 0;
    
    // Sub-seconds
    if (ct->ms >= 1000) {
        result |= (1 << 0);
    }
    // Seconds
    if (ct->s >= 60) {
        result |= (1 << 1);
    }
    // Minutes
    if (ct->m >= 60) {
        result |= (1 << 2);
    }
    // Hours
    if (ct->h >= 24) {
        result |= (1 << 3);
    }
    // Months
    if (ct->mo == 0 || ct->mo > 12) {
        result |= (1 << 4);
    }
    // Days
    if (ct->d == 0 || ct->d > days_in_month(ct)) {
        result |= (1 << 5);
    }
    return result;
}

// Calculate week-of-day applying Zeller's congruence
// ... https://en.wikipedia.org/wiki/Zeller%27s_congruence
dow_t dayofweek(ctime_t* ct) {
    uint8_t y = 10 * ct->yh + ct->yl;
    uint8_t mo = ct->mo;
    uint8_t d = ct->d;
    uint8_t z;
    
    if (mo == 1 || mo == 2) {
        y--;
        mo += 12;
    }
    z = y + (y / 4) + (20 / 4) - (20 * 2) + (13 * (mo + 1) / 5) + d;
    return (dow_t) ((z + 6) % 7 + 1);
}
