/*
 * nmea.c
 *
 *  Author: kayekss
 *  Target: unspecified
 */ 

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <avr/io.h>
#include "ctime.h"
#include "nmea.h"

// Look-up table for power of ten
uint16_t const lut_pow10[] = { 1, 10, 100, 1000, 10000 };

// Check isdigit() to multiple characters
// ... return true if all checked characters are decimal digit, false otherwise
inline bool isdigitn(uint8_t* s, uint16_t n) {
    bool result = true;

    for (uint16_t i = 0; i < n; i++) {
        if (!isdigit(*s++)) {
            result = false;
        }
    }
    return result;
}

// Check isxdigit() to multiple characters
// ... return true if all checked characters are hexadecimal digit, false otherwise
inline bool isxdigitn(uint8_t* s, uint16_t n) {
    bool result = true;

    for (uint16_t i = 0; i < n; i++) {
        if (!isxdigit(*s++)) {
            result = false;
        }
    }
    return result;
}

// Count consecutive decimal digits in string
inline uint16_t consecutive_digits(uint8_t* s) {
    uint16_t result = 0;

    while (*s && isdigit(*s++)) {
        result++;
    }
    return result;
}

// Convert hexadecimal digit character to binary
inline uint8_t c2b(uint8_t c) {
    uint8_t result = 0;

    if (c >= '0' && c <= '9') {
        result = c - '0';
    } else if (c >= 'A' && c <= 'F') {
        result = 10 + c - 'A';
    } else if (c >= 'a' && c <= 'f') {
        result = 10 + c - 'a';
    }
    return result;
}

// Parse and validate $__GGA message and pack to data structure
// ... return true if the message is invalid, false if valid
bool parse_gga(gga_t* gga, uint8_t* s, uint16_t count) {
    bool valid = true;
    gga_t gga0;
    uint8_t fn = 0;
    uint8_t field_count = 0;
    uint8_t field[11];
    uint8_t checksum_expected, checksum_given = 0x00;
    uint8_t c, cd = 0, cp = 0;

    checksum_expected = 'G' ^ 'P' ^ 'G' ^ 'G' ^ 'A' ^ ',';
    for (uint16_t j = 0; j < count && (c = *s++) != '\n'; j++) {
        if (c == ',' || c == '*' || c == '\r') {
            field[field_count] = '\0';
            switch (fn) {
            case 0:
                // UTC clock time
                if (valid) {
                    valid = field_count >= 6;
                    cp = 0;
                }
                if (valid) {
                    valid = isdigitn(&field[cp], 6);
                }
                if (valid) {
                    gga0.ct.h = c2b(field[cp]) * 10 + c2b(field[cp + 1]);
                    gga0.ct.m = c2b(field[cp + 2]) * 10 + c2b(field[cp + 3]);
                    gga0.ct.s = c2b(field[cp + 4]) * 10 + c2b(field[cp + 5]);
                    cp += 6;
                    gga0.ct.ms = 0;
                    valid = gga0.ct.h <= 23 && gga0.ct.m <= 59 &&
                        gga0.ct.s <= 59;
                }
                if (valid && field_count >= 7 && field[cp++] == '.') {
                    cd = consecutive_digits(&field[cp]);
                    
                    for (uint16_t i = 0; i < cd; i++) {
                        gga0.ct.ms += (uint16_t) c2b(field[cp++])
                            * lut_pow10[2 - i];
                    }
                }
                break;
            case 1:
                // Absolute value of latitude (optional)
                if (valid) {
                    valid = field_count == 0 || field_count >= 6;
                    cp = 0;
                }
                if (valid) {
                    if (field_count) {
                        cd = consecutive_digits(&field[cp]);
                        valid = cd <= 5;
                    }
                    gga0.latitude.integer = 0;
                    gga0.latitude.fraction = 0;
                }
                if (valid && field_count) {
                    for (uint16_t i = cd - 1; i < cd; i--) {
                        gga0.latitude.integer += c2b(field[cp++])
                            * lut_pow10[i];
                    }
                    if (field[cp++] == '.') {
                        cd = consecutive_digits(&field[cp]);
                        for (uint16_t i = 0; i < cd && i < 4; i++) {
                            gga0.latitude.fraction += c2b(field[cp++])
                                * lut_pow10[3 - i];
                        }
                    }
                }
                break;
            case 2:
                // Direction of latitude (optional)
                if (valid) {
                    valid = field_count == 0 || field_count == 1;
                    cp = 0;
                }
                if (valid) {
                    if (field_count) {
                        gga0.latitude.direction = field[cp++];
                    } else {
                        gga0.latitude.direction = 'N';
                    }
                }
                break;
            case 3:
                // Absolute value of latitude (optional)
                if (valid) {
                    valid = field_count == 0 || field_count >= 6;
                    cp = 0;
                }
                if (valid) {
                    if (field_count) {
                        cd = consecutive_digits(&field[cp]);
                        valid = cd <= 5;
                    }
                    gga0.longitude.integer = 0;
                    gga0.longitude.fraction = 0;
                }
                if (valid && field_count) {
                    for (uint16_t i = cd - 1; i < cd; i--) {
                        gga0.longitude.integer += c2b(field[cp++])
                            * lut_pow10[i];
                    }
                    if (field[cp++] == '.') {
                        cd = consecutive_digits(&field[cp]);
                        for (uint16_t i = 0; i < cd && i < 4; i++) {
                            gga0.longitude.fraction += c2b(field[cp++])
                                * lut_pow10[3 - i];
                        }
                    }
                }
                break;
            case 4:
                // Direction of longitude (optional)
                if (valid) {
                    valid = field_count == 0 || field_count == 1;
                    cp = 0;
                }
                if (valid) {
                    if (field_count) {
                        gga0.longitude.direction = field[cp++];
                    } else {
                        gga0.longitude.direction = 'E';
                    }
                }
                break;
            case 5:
                // Position fix status
                if (valid) {
                    valid = field_count == 1;
                    cp = 0;
                }
                if (valid) {
                    valid = isdigit(field[cp]);
                }
                if (valid) {
                    gga0.status = c2b(field[cp++]);
                }
                break;
            case 6:
                // Satellites in use (optional)
                if (valid) {
                    valid = field_count == 0 || field_count <= 3;
                    cp = 0;
                }
                if (valid) {
                    if (field_count) {
                        cd = consecutive_digits(&field[cp]);
                        valid = cd >= 1 && cd <= 3;
                    }
                    gga0.sats_in_use = 0;
                }
                if (valid && field_count) {
                    for (uint16_t i = cd - 1; i < cd; i--) {
                        gga0.sats_in_use += c2b(field[cp++]) * lut_pow10[i];
                    }
                }
                break;
            case 7:
                // HDOP (optional)
                if (valid) {
                    valid = field_count == 0 || field_count <= 10;
                    cp = 0;
                }
                if (valid) {
                    if (field_count) {
                        cd = consecutive_digits(&field[cp]);
                        valid = cd <= 5;
                    }
                    gga0.hdop.integer = 0;
                    gga0.hdop.fraction = 0;
                }
                if (valid && field_count) {
                    for (uint16_t i = cd - 1; i < cd; i--) {
                        gga0.hdop.integer += c2b(field[cp++])
                            * lut_pow10[i];
                    }
                    if (field[cp++] == '.') {
                        cd = consecutive_digits(&field[cp]);
                        for (uint16_t i = 0; i < cd && i < 4; i++) {
                            gga0.hdop.fraction += c2b(field[cp++])
                                * lut_pow10[3 - i];
                        }
                    }
                }
                break;
            case 8:
                // Value of MSL orthometric height (optional)
                if (valid) {
                    valid = field_count == 0 || field_count <= 10;
                    cp = 0;
                }
                if (valid) {
                    if (field_count) {
                        valid = isdigit(field[cp]) || field[cp] == '-';
                        if (field[cp] == '-') {
                            gga0.height.sign = 1;
                            cp += 1;
                        } else {
                            gga0.height.sign = 0;
                        }
                        cd = consecutive_digits(&field[cp]);
                        valid = valid && cd <= 5;
                    } else {
                        gga0.height.sign = 0;
                    }
                    gga0.height.integer = 0;
                    gga0.height.fraction = 0;
                }
                if (valid && field_count) {
                    for (uint16_t i = cd - 1; i < cd; i--) {
                        gga0.height.integer += c2b(field[cp++])
                            * lut_pow10[i];
                    }
                    if (field[cp++] == '.') {
                        cd = consecutive_digits(&field[cp]);
                        for (uint16_t i = 0; i < cd && i < 4; i++) {
                            gga0.height.fraction += c2b(field[cp++])
                                * lut_pow10[3 - i];
                        }
                    }
                }
                break;
            case 9:
                // Units of MSL orthometric height (optional)
                if (valid) {
                    valid = field_count == 0 || field_count == 1;
                    cp = 0;
                }
                if (valid) {
                    if (field_count) {
                        gga0.height.units= field[cp++];
                    } else {
                        gga0.height.units = 'M';
                    }
                }
                break;
            case 10:
                // Value of geoid separation (optional)
                if (valid) {
                    valid = field_count == 0 || field_count <= 10;
                    cp = 0;
                }
                if (valid) {
                    if (field_count) {
                        valid = isdigit(field[cp]) || field[cp] == '-';
                        if (field[cp] == '-') {
                            gga0.geoid_separation.sign = 1;
                            cp += 1;
                        } else {
                            gga0.geoid_separation.sign = 0;
                        }
                        cd = consecutive_digits(&field[cp]);
                        valid = valid && cd <= 5;
                    } else {
                        gga0.geoid_separation.sign = 0;
                    }
                    gga0.geoid_separation.integer = 0;
                    gga0.geoid_separation.fraction = 0;
                }
                if (valid && field_count) {
                    for (uint16_t i = cd - 1; i < cd; i--) {
                        gga0.geoid_separation.integer += c2b(field[cp++])
                            * lut_pow10[i];
                    }
                    if (field[cp++] == '.') {
                        cd = consecutive_digits(&field[cp]);
                        for (uint16_t i = 0; i < cd && i < 4; i++) {
                            gga0.geoid_separation.fraction += c2b(field[cp++])
                                * lut_pow10[3 - i];
                        }
                    }
                }
                break;
            case 11:
                // Units of geoid separation (optional)
                if (valid) {
                    valid = field_count == 0 || field_count == 1;
                    cp = 0;
                }
                if (valid) {
                    if (field_count) {
                        gga0.geoid_separation.units= field[cp++];
                    } else {
                        gga0.geoid_separation.units = 'M';
                    }
                }
                break;
            case 12:
                // Age of DGPS data in seconds (optional)
                if (valid) {
                    valid = field_count == 0 || field_count <= 10;
                    cp = 0;
                }
                if (valid) {
                    if (field_count) {
                        cd = consecutive_digits(&field[cp]);
                        valid = cd <= 5;
                    }
                    gga0.dgps_age.integer = 0;
                    gga0.dgps_age.fraction = 0;
                }
                if (valid && field_count) {
                    for (uint16_t i = cd - 1; i < cd; i--) {
                        gga0.dgps_age.integer += c2b(field[cp++])
                            * lut_pow10[i];
                    }
                    if (field[cp++] == '.') {
                        cd = consecutive_digits(&field[cp]);
                        for (uint16_t i = 0; i < cd && i < 4; i++) {
                            gga0.dgps_age.fraction += c2b(field[cp++])
                                * lut_pow10[3 - i];
                        }
                    }
                }
                break;
            case 13:
                // DGPS reference station ID (optional)
                if (valid) {
                    valid = field_count == 0 || field_count == 4;
                    cp = 0;
                }
                if (valid) {
                    if (field_count) {
                        valid = isxdigitn(&field[cp], 4);
                    } else {
                        gga0.dgps_station_id = 0x0000;
                    }
                }
                if (valid && field_count) {
                    gga0.dgps_station_id = (uint16_t) c2b(field[cp]) << 12
                        | (uint16_t) c2b(field[cp + 1]) << 8
                        | (uint16_t) c2b(field[cp + 2]) << 4
                        | c2b(field[cp + 3]);
                    cp += 4;
                }
                break;
            case 14:
                // Checksum
                if (valid) {
                    valid = field_count == 2;
                    cp = 0;
                }
                if (valid) {
                    valid = isxdigitn(&field[cp], 2);
                }
                if (valid) {
                    checksum_given = c2b(field[cp]) * 16 + c2b(field[cp + 1]);
                    cp += 2;
                }
                break;
            default:
                break;
            }
            fn++;
            field_count = 0;
        } else {
            field[field_count++] = c;
        }
        // Calculate checksum
        if (fn < 14) {
            checksum_expected ^= c;
        }
    }
    gga0.checksum_expected = checksum_expected;
    gga0.checksum_given = checksum_given;
    if (valid) {
        valid = checksum_given == checksum_expected;
    }
    if (valid) {
        *gga = gga0;
    }
    return valid == false;
}

// Parse and validate $__ZDA message and get represented clock time
// ... return true if the message is invalid, false if valid
bool parse_zda(zda_t* zda, uint8_t* s, uint16_t count) {
    bool valid = true;
    zda_t zda0;
    struct {
        bool sign;
        uint8_t h, m;
    } ct_offset = { 0, 0, 0 };
    uint8_t fn = 0;
    uint8_t field_count = 0;
    uint8_t field[11];
    uint8_t checksum_expected, checksum_given = 0x00;
    uint8_t c, cd = 0, cp = 0;

    checksum_expected = 'G' ^ 'P' ^ 'Z' ^ 'D' ^ 'A' ^ ',';
    for (uint16_t i = 0; i < count && (c = *s++) != '\n'; i++) {
        if (c == ',' || c == '*' || c == '\r') {
            field[field_count] = '\0';
            switch (fn) {
            case 0:
                // Hours, minutes, seconds, sub-seconds (optional)
                if (valid) {
                    valid = field_count >= 6;
                    cp = 0;
                }
                if (valid) {
                    valid = isdigitn(&field[cp], 6);
                }
                if (valid) {
                    zda0.ct.h = c2b(field[cp]) * 10 + c2b(field[cp + 1]);
                    zda0.ct.m = c2b(field[cp + 2]) * 10 + c2b(field[cp + 3]);
                    zda0.ct.s = c2b(field[cp + 4]) * 10 + c2b(field[cp + 5]);
                    cp += 6;
                    zda0.ct.ms = 0;
                    valid = zda0.ct.h <= 23 && zda0.ct.m <= 59 &&
                        zda0.ct.s <= 59;
                }
                if (valid && field_count >= 7 && field[cp++] == '.') {
                    cd = consecutive_digits(&field[cp]);
                    for (uint16_t i = 0; i < cd; i++) {
                        zda0.ct.ms += (uint16_t) c2b(field[cp++])
                            * lut_pow10[2 - i];
                    }
                }
                break;
            case 1:
                // Days
                if (valid) {
                    valid = field_count == 2;
                    cp = 0;
                }
                if (valid) {
                    valid = isdigitn(&field[cp], 2);
                }
                if (valid) {
                    zda0.ct.d = c2b(field[cp]) * 10 + c2b(field[cp + 1]);
                    cp += 2;
                }
                break;
            case 2:
                // Months
                if (valid) {
                    valid = field_count == 2;
                    cp = 0;
                }
                if (valid) {
                    valid = isdigitn(&field[cp], 2);
                }
                if (valid) {
                    zda0.ct.mo = c2b(field[cp]) * 10 + c2b(field[cp + 1]);
                    cp += 2;
                    valid = zda0.ct.mo >= 1 && zda0.ct.mo <= 12;
                }
                break;
            case 3:
                // Years
                if (valid) {
                    valid = field_count == 4;
                    cp = 0;
                }
                if (valid) {
                    valid = isdigitn(&field[cp], 4);
                }
                if (valid) {
                    cp += 2;
                    zda0.ct.yh = c2b(field[cp++]);
                    zda0.ct.yl = c2b(field[cp++]);
                    valid = zda0.ct.yh <= 9 && zda0.ct.yl <= 9 &&
                        zda0.ct.d >= 1 && zda0.ct.d <= days_in_month(&zda0.ct);
                }
                break;
            case 4:
                // Local time offset hours (optional)
                if (valid) {
                    valid = field_count == 0 || field_count == 2
                        || field_count == 3;
                    cp = 0;
                }
                if (valid) {
                    if (field_count) {
                        valid = isdigit(field[cp]) || field[cp] == '-';
                        if (field[cp] == '-') {
                            ct_offset.sign = 1;
                            cp += 1;
                        } else {
                            ct_offset.sign = 0;
                        }
                        cd = consecutive_digits(&field[cp]);
                        valid = valid && cd == 2;
                    } else {
                        ct_offset.sign = 0;
                    }
                }
                if (valid && field_count) {
                    ct_offset.h = c2b(field[cp]) * 10 + c2b(field[cp + 1]);
                    cp += 2;
                    valid = ct_offset.h <= 13;
                }
                break;
            case 5:
                // Local time offset minutes (optional)
                if (valid) {
                    valid = field_count == 0 || field_count == 2;
                    cp = 0;
                }
                if (valid) {
                    if (field_count) {
                        valid = isdigitn(&field[cp], 2);
                    } else {
                        ct_offset.m = 0;
                    }
                }
                if (valid && field_count) {
                    ct_offset.m = c2b(field[cp]) * 10 + c2b(field[cp + 1]);
                    cp += 2;
                    valid = ct_offset.m <= 59;
                }
                break;
            case 6:
                // Checksum
                if (valid) {
                    valid = field_count == 2;
                    cp = 0;
                }
                if (valid) {
                    valid = isxdigitn(&field[cp], 2);
                }
                if (valid) {
                    checksum_given = c2b(field[cp]) * 16 + c2b(field[cp + 1]);
                    cp += 2;
                }
                break;
            default:
                break;
            }
            fn++;
            field_count = 0;
        } else {
            field[field_count++] = c;
        }
        // Calculate checksum
        if (fn < 6) {
            checksum_expected ^= c;
        }
    }
    zda0.checksum_expected = checksum_expected;
    zda0.checksum_given = checksum_given;
    if (valid) {
        valid = checksum_given == checksum_expected;
        // Force set local time to JST (UTC+9)
        ct_offset.sign = 0;
        ct_offset.h = 9;
        ct_offset.m = 0;
    }
    if (valid) {
        // Convert UTC time to local time
        if (ct_offset.sign) {
            if (zda0.ct.m < ct_offset.m) {
                zda0.ct.m = zda0.ct.m + 60 - ct_offset.m;
                ct_offset.h++;
            } else {
                zda0.ct.m -= ct_offset.m;
            }
            if (zda0.ct.h < ct_offset.h) {
                zda0.ct.h = zda0.ct.h + 24 - ct_offset.h;
                ctime_decrement_day(&zda0.ct);
            } else {
                zda0.ct.h -= ct_offset.h;
            }
        } else {
            if (zda0.ct.m + ct_offset.m >= 60) {
                zda0.ct.m = zda0.ct.m + ct_offset.m - 60;
                ct_offset.h++;
            } else {
                zda0.ct.m += ct_offset.m;
            }
            if (zda0.ct.h + ct_offset.h >= 24) {
                zda0.ct.h = zda0.ct.h + ct_offset.h - 24;
                ctime_increment_day(&zda0.ct);
            } else {
                zda0.ct.h += ct_offset.h;
            }
        }
        *zda = zda0;
    }
    return valid == false;
}
