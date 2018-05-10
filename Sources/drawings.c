/*
 * DotMatrixClock2018/drawings.c
 *
 *  Author: kayekss
 *  Target: ATmega328P, 20.000 MHz crystal oscillator
 */

#include <stdint.h>
#include <stdbool.h>
#include "ctime.h"
#include "event.h"
#include "display.h"
#include "drawings.h"

extern volatile uint32_t gbuf_back[16];
// 6 Clock digits
cdigit_t cd[6] = {
    { ' ', ' ', 0 },
    { ' ', ' ', 0 },
    { ' ', ' ', 0 },
    { ' ', ' ', 0 },
    { ' ', ' ', 0 },
    { ' ', ' ', 0 }
};

// Update clock digits to internal clock time
void update_cdigit(ctime_t* ct) {
    // Set characters, hour high digit is zero-suppressed
    cd[0].chr = (ct->h / 10) ? '0' + ct->h / 10 : ' ';
    cd[1].chr = '0' + ct->h % 10;
    cd[2].chr = '0' + ct->m / 10;
    cd[3].chr = '0' + ct->m % 10;
    cd[4].chr = '0' + ct->s / 10;
    cd[5].chr = '0' + ct->s % 10;

    // Reset unused scrolling characters and frame counts
    for (uint8_t i = 0; i < 6; i++) {
        cd[i].chr_scroll = cd[i].chr;
        cd[i].frame = 0;
    }
}

// Update clock digits to internal clock time, with vertical scrolls
void update_cdigit_with_scroll(ctime_t* ct) {
    uint8_t chr_t[6];

    // Set characters, hour high digit is zero-suppressed
    chr_t[0] = (ct->h / 10) ? '0' + ct->h / 10 : ' ';
    chr_t[1] = '0' + ct->h % 10;
    chr_t[2] = '0' + ct->m / 10;
    chr_t[3] = '0' + ct->m % 10;
    chr_t[4] = '0' + ct->s / 10;
    chr_t[5] = '0' + ct->s % 10;

    // Update scrolling characters and frame counts
    for (uint8_t i = 0; i < 6; i++) {
        if (cd[i].frame == 0) {
            // Start scrolling when the character is changed
            if (chr_t[i] != cd[i].chr) {
                cd[i].chr_scroll = chr_t[i];
            }
        } else {
            // Restart scrolling when the character is changed
            if (chr_t[i] != cd[i].chr_scroll) {
                cd[i].chr = cd[i].chr_scroll;
                cd[i].chr_scroll = chr_t[i];
                cd[i].frame = 0;
            }
        }
        // Advance frame
        if (cd[i].chr != cd[i].chr_scroll) {
            cd[i].frame++;
        }
        // Done scrolling when frame count reaches to the font height
        if (cd[i].frame > (i < 4 ? 10 : 5)) {
            cd[i].frame = 0;
            cd[i].chr = cd[i].chr_scroll;
        }
    }
}

// Draw clock time in "{hours}:{minutes}" format (no second digits)
void draw_time_hm(ctime_t* ct, uint16_t mask) {
    // Bit mask of drawable elements
    //   mask<15>    reserved
    //       <14>    colon
    //       <13:8>  reserved
    //       <7>     hours
    //       <6>     high digit of minutes
    //       <5>     low digit of minutes
    //       <4:0>   reserved

    font_t const font_list[4] = {
        FONT_M0610, FONT_M0610, FONT_M0610, FONT_M0610
    };
    uint8_t const x_list[4] = { 30, 23, 13, 6 };
    uint8_t const y_list[4] = { 6, 6, 6, 6 };
    uint8_t const mask_list[4] = { 7, 7, 6, 5 };

    // Update clock characters' scroll states
    update_cdigit_with_scroll(ct);
    // Draw colon
    if (mask & (1 << 14)) {
        display_putc(FONT_M0410, 16, 6, ':');
    }
    // Draw digits
    for (uint8_t i = 0; i < 4; i++) {
        if (mask & (1 << mask_list[i])) {
            display_putc_scroll(font_list[i], x_list[i], y_list[i],
                cd[i].chr, cd[i].chr_scroll, cd[i].frame);
        }
    }
}

// Draw clock time in "{hours}:{minutes} {seconds}" format
void draw_time_hms(ctime_t* ct, uint16_t mask) {
    // Bit mask of drawable elements
    //   mask<15>    reserved
    //       <14>    colon
    //       <13:8>  reserved
    //       <7>     hours
    //       <6>     high digit of minutes
    //       <5>     low digit of minutes
    //       <4>     high digit of seconds
    //       <3>     low digit of seconds
    //       <2:0>   reserved

    font_t const font_list[6] = {
        FONT_M0410, FONT_M0410, FONT_M0410, FONT_M0410,
        FONT_PP05, FONT_PP05
    };
    uint8_t const x_list[6] = { 31, 26, 18, 13, 6, 2 };
    uint8_t const y_list[6] = { 6, 6, 6, 6, 11, 11 };
    uint8_t const mask_list[6] = { 7, 7, 6, 5, 4, 3 };

    // Update clock characters' scroll states
    update_cdigit_with_scroll(ct);
    // Draw colon
    if (mask & (1 << 14)) {
        display_putc(FONT_M0410, 21, 6, ':');
    }
    // Draw digits
    for (uint8_t i = 0; i < 6; i++) {
        if (mask & (1 << mask_list[i])) {
            display_putc_scroll(font_list[i], x_list[i], y_list[i],
                cd[i].chr, cd[i].chr_scroll, cd[i].frame);
        }
    }
}

// Draw clock date in "{months}/{days} {day-of-week}" format
void draw_date_dayofweek(ctime_t* ct, dow_t dow, uint16_t mask) {
    // Bit mask of drawable elements
    //   mask<15>     slash between months and days
    //       <14:11>  reserved
    //       <10>     months
    //       <9>      days
    //       <8>      day-of-week string
    //       <7:0>    reserved

    // Draw slash
    if (mask & (1 << 15)) {
        display_putc(FONT_PP05, 24, 0, '/');
    }
    // Draw months
    if (mask & (1 << 10)) {
        if (ct->mo >= 10) {
            display_putc(FONT_PP05, 31, 0, '1');
            display_putc(FONT_PP05, 28, 0, '0' + ct->mo - 10);
        } else {
            display_putc(FONT_PP05, 29, 0, '0' + ct->mo);
        }
    }
    // Draw days
    if (mask & (1 << 9)) {
        if (ct->d >= 10) {
            display_putc(FONT_PP05, 21, 0, '0' + ct->d / 10);
            display_putc(FONT_PP05, 17, 0, '0' + ct->d % 10);
        } else {
            display_putc(FONT_PP05, 19, 0, '0' + ct->d);
        }
    }
    // Draw day-of-week string
    if (mask & (1 << 8)) {
        switch (dow) {
        case DOW_SUNDAY:
            display_putc(FONT_PP05, 10, 0, 'S');
            display_putc(FONT_PP05, 6, 0, 'u');
            display_putc(FONT_PP05, 2, 0, 'n');
            break;
        case DOW_MONDAY:
            display_putc(FONT_PP05, 12, 0, 'M');
            display_putc(FONT_PP05, 6, 0, 'o');
            display_putc(FONT_PP05, 2, 0, 'n');
            break;
        case DOW_TUESDAY:
            display_putc(FONT_PP05, 10, 0, 'T');
            display_putc(FONT_PP05, 6, 0, 'u');
            display_putc(FONT_PP05, 2, 0, 'e');
            break;
        case DOW_WEDNESDAY:
            display_putc(FONT_PP05, 12, 0, 'W');
            display_putc(FONT_PP05, 6, 0, 'e');
            display_putc(FONT_PP05, 2, 0, 'd');
            break;
        case DOW_THURSDAY:
            display_putc(FONT_PP05, 10, 0, 'T');
            display_putc(FONT_PP05, 6, 0, 'h');
            display_putc(FONT_PP05, 2, 0, 'u');
            break;
        case DOW_FRIDAY:
            display_putc(FONT_PP05, 7, 0, 'F');
            display_putc(FONT_PP05, 3, 0, 'r');
            display_putc(FONT_PP05, 0, 0, 'i');
            break;
        case DOW_SATURDAY:
            display_putc(FONT_PP05, 9, 0, 'S');
            display_putc(FONT_PP05, 5, 0, 'a');
            display_putc(FONT_PP05, 1, 0, 't');
            break;
        }
    }
}

// Draw clock date in "{month string} {days} '{years}" format
void draw_date_year(ctime_t* ct, uint16_t mask) {
    // Bit mask of drawable elements
    //   mask<15>     apostrophe for year abbreviation
    //       <14:13>  reserved
    //       <12>     high digit of years
    //       <11>     low digit of years
    //       <10>     month string
    //       <9>      days
    //       <8:0>    reserved

    uint8_t dh, dl;

    // Draw month string
    if (mask & (1 << 10)) {
        switch (ct->mo) {
        case 1:  // Jan
            display_putc(FONT_PP05, 31, 0, 'J');
            display_putc(FONT_PP05, 27, 0, 'a');
            display_putc(FONT_PP05, 23, 0, 'n');
            break;
        case 2:  // Feb
            display_putc(FONT_PP05, 31, 0, 'F');
            display_putc(FONT_PP05, 27, 0, 'e');
            display_putc(FONT_PP05, 23, 0, 'b');
            break;
        case 3:  // Mar
            display_putc(FONT_PP05, 31, 0, 'M');
            display_putc(FONT_PP05, 25, 0, 'a');
            display_putc(FONT_PP05, 21, 0, 'r');
            break;
        case 4:  // Apr
            display_putc(FONT_PP05, 31, 0, 'A');
            display_putc(FONT_PP05, 27, 0, 'p');
            display_putc(FONT_PP05, 23, 0, 'r');
            break;
        case 5:  // May
            display_putc(FONT_PP05, 31, 0, 'M');
            display_putc(FONT_PP05, 25, 0, 'a');
            display_putc(FONT_PP05, 21, 0, 'y');
            break;
        case 6:  // Jun
            display_putc(FONT_PP05, 31, 0, 'J');
            display_putc(FONT_PP05, 27, 0, 'u');
            display_putc(FONT_PP05, 23, 0, 'n');
            break;
        case 7:  // Jul
            display_putc(FONT_PP05, 31, 0, 'J');
            display_putc(FONT_PP05, 27, 0, 'u');
            display_putc(FONT_PP05, 23, 0, 'l');
            break;
        case 8:  // Aug
            display_putc(FONT_PP05, 31, 0, 'A');
            display_putc(FONT_PP05, 27, 0, 'u');
            display_putc(FONT_PP05, 23, 0, 'g');
            break;
        case 9:  // Sep
            display_putc(FONT_PP05, 31, 0, 'S');
            display_putc(FONT_PP05, 27, 0, 'e');
            display_putc(FONT_PP05, 23, 0, 'p');
            break;
        case 10:  // Oct
            display_putc(FONT_PP05, 31, 0, 'O');
            display_putc(FONT_PP05, 27, 0, 'c');
            display_putc(FONT_PP05, 23, 0, 't');
            break;
        case 11:  // Nov
            display_putc(FONT_PP05, 31, 0, 'N');
            display_putc(FONT_PP05, 26, 0, 'o');
            display_putc(FONT_PP05, 22, 0, 'v');
            break;
        case 12:  // Dec
            display_putc(FONT_PP05, 31, 0, 'D');
            display_putc(FONT_PP05, 27, 0, 'e');
            display_putc(FONT_PP05, 23, 0, 'c');
            break;
        default:
            display_putc(FONT_PP05, 31, 0, '-');
            display_putc(FONT_PP05, 27, 0, '-');
            display_putc(FONT_PP05, 23, 0, '-');
            break;
        }
    }
    // Draw days
    if (mask & (1 << 9)) {
        if (ct->d >= 10) {
            if (ct->d >= 30) {
                dh = 3;
                dl = ct->d - 30;
            } else if (ct->d >= 20) {
                dh = 2;
                dl = ct->d - 20;
            } else {
                dh = 1;
                dl = ct->d - 10;
            }
            display_putc(FONT_PP05, 17, 0, '0' + dh);
            display_putc(FONT_PP05, 13, 0, '0' + dl);
        } else {
            display_putc(FONT_PP05, 15, 0, '0' + ct->d);
        }
    }
    // Draw apostrophe
    if (mask & (1 << 15)) {
        display_putc(FONT_PP05, 8, 0, '\'');
    }
    // Draw years
    if (mask & (1 << 12)) {
        display_putc(FONT_PP05, 6, 0, '0' + ct->yh);
    }
    if (mask & (1 << 11)) {
        display_putc(FONT_PP05, 2, 0, '0' + ct->yl);
    }
}

// Draw temperature sensor status
void draw_temperature(bool result, uint8_t sign, uint8_t integer,
    uint16_t fraction_x10k, uint16_t mask) {
    // Bit mask of drawable elements
    //   mask<15:3>  reserved
    //       <2>     temperature value
    //       <1:0>   reserved

    // Draw temperature
    if (mask & (1 << 2)) {
        if (result == 0) {
            // Sign and integer part
            if (integer >= 100) {
                if (sign) {
                    display_putc(FONT_PP05, 29, 0, '-');
                }
                display_putc(FONT_PP05, 25, 0, '0' + integer / 100);
                display_putc(FONT_PP05, 21, 0, '0' + integer / 10 % 10);
                display_putc(FONT_PP05, 17, 0, '0' + integer % 10);
            } else if (integer >= 10) {
                if (sign) {
                    display_putc(FONT_PP05, 25, 0, '-');
                }
                display_putc(FONT_PP05, 21, 0, '0' + integer / 10);
                display_putc(FONT_PP05, 17, 0, '0' + integer % 10);
            } else {
                if (sign) {
                    display_putc(FONT_PP05, 21, 0, '-');
                }
                display_putc(FONT_PP05, 17, 0, '0' + integer);
            }
            // Dot and fraction part
            display_putc(FONT_PP05, 13, 0, '.');
            display_putc(FONT_PP05, 11, 0, '0' + fraction_x10k / 1000);
            // Degree sign
            display_putc(FONT_PP05, 6, 0, '\177');
            display_putc(FONT_PP05, 3, 0, 'C');
        } else {
            display_putc(FONT_PP05, 11, 0, '-');
            display_putc(FONT_PP05, 7, 0, '-');
            display_putc(FONT_PP05, 3, 0, '-');
        }                            
    }
}

// Draw GPS tracking status
void draw_gps_status(uint8_t fix, uint8_t blink_phase, uint16_t mask) {
    // Bit mask of drawable elements
    //   mask<15:3>  reserved
    //       <2>     GPS tracking status "No GPS"/"Unfixed"/"2D fix"/"3D fix"
    //       <1:0>   reserved

    // Draw GPS tracking status
    if (mask & (1 << 2)) {
        switch (fix) {
        case 0xff:
            display_putc(FONT_PP05, 31, 0, '-');
            display_putc(FONT_PP05, 27, 0, '-');
            display_putc(FONT_PP05, 23, 0, '-');
            break;
        case 0x00:
            display_putc(FONT_PP05, 31, 0, '\232' + blink_phase);
            display_putc(FONT_PP05, 28, 0, '\226');
            display_putc(FONT_PP05, 20, 0, '\227');
            display_putc(FONT_PP05, 12, 0, '\230');
            display_putc(FONT_PP05, 4, 0, '\231');
            break;
        case 0x01:
            display_putc(FONT_PP05, 33, 1, '\205');
            display_putc(FONT_PP05, 25, 0, 'G');
            display_putc(FONT_PP05, 21, 0, 'P');
            display_putc(FONT_PP05, 17, 0, 'S');
            break;
        case 0x02:
            display_putc(FONT_PP05, 33, 1, '\205');
            display_putc(FONT_PP05, 25, 0, 'D');
            display_putc(FONT_PP05, 21, 0, 'G');
            display_putc(FONT_PP05, 17, 0, 'P');
            display_putc(FONT_PP05, 13, 0, 'S');
            break;
        case 0x03:
            display_putc(FONT_PP05, 33, 1, '\205');
            display_putc(FONT_PP05, 25, 0, 'P');
            display_putc(FONT_PP05, 21, 0, 'P');
            display_putc(FONT_PP05, 17, 0, 'S');
            break;
        case 0x04:
            display_putc(FONT_PP05, 33, 1, '\205');
            display_putc(FONT_PP05, 25, 0, 'R');
            display_putc(FONT_PP05, 21, 0, 'T');
            display_putc(FONT_PP05, 17, 0, 'K');
            break;
        case 0x05:
            display_putc(FONT_PP05, 33, 1, '\205');
            display_putc(FONT_PP05, 25, 0, 'F');
            display_putc(FONT_PP05, 21, 0, 'l');
            display_putc(FONT_PP05, 18, 0, '.');
            display_putc(FONT_PP05, 15, 0, 'R');
            display_putc(FONT_PP05, 11, 0, 'T');
            display_putc(FONT_PP05, 7, 0, 'K');
            break;
        case 0x06:
            display_putc(FONT_PP05, 33, 1, '\205');
            display_putc(FONT_PP05, 25, 0, 'D');
            display_putc(FONT_PP05, 21, 0, '.');
            display_putc(FONT_PP05, 18, 0, 'R');
            display_putc(FONT_PP05, 14, 0, 'e');
            display_putc(FONT_PP05, 10, 0, 'c');
            display_putc(FONT_PP05, 6, 0, 'k');
            display_putc(FONT_PP05, 2, 0, '.');
            break;
        case 0x07:
            display_putc(FONT_PP05, 33, 1, '\205');
            display_putc(FONT_PP05, 25, 0, 'M');
            display_putc(FONT_PP05, 19, 0, 'a');
            display_putc(FONT_PP05, 15, 0, 'n');
            display_putc(FONT_PP05, 11, 0, 'u');
            display_putc(FONT_PP05, 7, 0, 'a');
            display_putc(FONT_PP05, 3, 0, 'l');
            break;
        case 0x08:
            display_putc(FONT_PP05, 33, 1, '\205');
            display_putc(FONT_PP05, 25, 0, 'S');
            display_putc(FONT_PP05, 21, 0, 'i');
            display_putc(FONT_PP05, 19, 0, 'm');
            display_putc(FONT_PP05, 13, 0, 'u');
            display_putc(FONT_PP05, 9, 0, 'l');
            display_putc(FONT_PP05, 6, 0, '.');
            break;
        default:
            break;
        }
    }
}

// Draw configuration screen USE_GPS
void draw_config_use_gps(bool use_gps, uint16_t mask) {
    // Bit mask of drawable elements
    //   mask<15:7>  reserved
    //       <6>     ballot box
    //       <5>     value string "yes"/"no"
    //       <4:3>   reserved
    //       <2>     caption string "Use GPS"
    //       <1>     left button icon <changevalue>
    //       <0>     right button icon <save>

    // Draw button icons
    if (mask & (1 << 1)) {
        display_putc(FONT_PP05, 27, 0, '\203');
    }
    if (mask & (1 << 0)) {
        display_putc(FONT_PP05, 11, 0, '\205');
    }
    // Draw caption string "Use GPS"
    if (mask & (1 << 2)) {
        display_putc(FONT_PP05, 31, 5, 'U');
        display_putc(FONT_PP05, 27, 5, 's');
        display_putc(FONT_PP05, 23, 5, 'e');
        display_putc(FONT_PP05, 17, 5, 'G');
        display_putc(FONT_PP05, 13, 5, 'P');
        display_putc(FONT_PP05, 9, 5, 'S');
    }
    // Draw ballot box
    if (mask & (1 << 6)) {
        display_putc(FONT_PP05, 17, 11, use_gps ? '\207' : '\206');
    }
    // Draw value string "yes"/"no"
    if (mask & (1 << 5)) {
        if (use_gps) {
            display_putc(FONT_PP05, 10, 11, 'y');
            display_putc(FONT_PP05, 6, 11, 'e');
            display_putc(FONT_PP05, 2, 11, 's');
        } else {
            display_putc(FONT_PP05, 9, 11, 'n');
            display_putc(FONT_PP05, 5, 11, 'o');
        }
    }
}

// Draw configuration screen SET_TIME_TOP
void draw_config_set_time_top(uint16_t mask) {
    // Bit mask of drawable elements
    //   mask<15:3>  reserved
    //       <2>     caption string "Set time manually"
    //       <1>     left button icon <down>
    //       <0>     right button icon <next>

    // Draw button icons
    if (mask & (1 << 1)) {
        display_putc(FONT_PP05, 27, 0, '\201');
    }
    if (mask & (1 << 0)) {
        display_putc(FONT_PP05, 11, 0, '\202');
    }
    // Draw caption string "Set time manually"
    if (mask & (1 << 2)) {
        display_putc(FONT_PP05, 31, 5, 'S');
        display_putc(FONT_PP05, 27, 5, 'e');
        display_putc(FONT_PP05, 23, 5, 't');
        display_putc(FONT_PP05, 19, 5, 't');
        display_putc(FONT_PP05, 16, 5, 'i');
        display_putc(FONT_PP05, 14, 5, 'm');
        display_putc(FONT_PP05, 8, 5, 'e');
        display_putc(FONT_PP05, 31, 11, 'm');
        display_putc(FONT_PP05, 25, 11, 'a');
        display_putc(FONT_PP05, 21, 11, 'n');
        display_putc(FONT_PP05, 17, 11, 'u');
        display_putc(FONT_PP05, 13, 11, 'a');
        display_putc(FONT_PP05, 9, 11, 'l');
        display_putc(FONT_PP05, 6, 11, 'l');
        display_putc(FONT_PP05, 3, 11, 'y');
    }
}

// Draw common configuration screen SET_TIME_MOD*
void draw_config_set_time_mod(ctime_t* ct, dow_t dow, uint8_t icon_l,
    uint8_t icon_r, uint16_t mask) {
    // Bit mask of drawable elements
    //   mask<15>   apostrophe for year abbreviation
    //       <14>   colon
    //       <13>   reserved
    //       <12>   high digit of years
    //       <11>   low digit of years
    //       <10>   month string
    //       <9>    days
    //       <8>    day-of-week string
    //       <7>    hours
    //       <6>    high digit of minutes
    //       <5>    low digit of minutes
    //       <4:2>  reserved
    //       <1>    left button icon
    //       <0>    right button icon

    // Draw button icons
    if (mask & (1 << 1)) {
        display_putc(FONT_PP05, 27, 0, icon_l);
    }
    if (mask & (1 << 0)) {
        display_putc(FONT_PP05, 11, 0, icon_r);
    }
    // Draw month string
    if (mask & (1 << 10)) {
        switch (ct->mo) {
        case 1:  // Jan
            display_putc(FONT_PP05, 31, 5, 'J');
            display_putc(FONT_PP05, 27, 5, 'a');
            display_putc(FONT_PP05, 23, 5, 'n');
            break;
        case 2:  // Feb
            display_putc(FONT_PP05, 31, 5, 'F');
            display_putc(FONT_PP05, 27, 5, 'e');
            display_putc(FONT_PP05, 23, 5, 'b');
            break;
        case 3:  // Mar
            display_putc(FONT_PP05, 31, 5, 'M');
            display_putc(FONT_PP05, 25, 5, 'a');
            display_putc(FONT_PP05, 21, 5, 'r');
            break;
        case 4:  // Apr
            display_putc(FONT_PP05, 31, 5, 'A');
            display_putc(FONT_PP05, 27, 5, 'p');
            display_putc(FONT_PP05, 23, 5, 'r');
            break;
        case 5:  // May
            display_putc(FONT_PP05, 31, 5, 'M');
            display_putc(FONT_PP05, 25, 5, 'a');
            display_putc(FONT_PP05, 21, 5, 'y');
            break;
        case 6:  // Jun
            display_putc(FONT_PP05, 31, 5, 'J');
            display_putc(FONT_PP05, 27, 5, 'u');
            display_putc(FONT_PP05, 23, 5, 'n');
            break;
        case 7:  // Jul
            display_putc(FONT_PP05, 31, 5, 'J');
            display_putc(FONT_PP05, 27, 5, 'u');
            display_putc(FONT_PP05, 23, 5, 'l');
            break;
        case 8:  // Aug
            display_putc(FONT_PP05, 31, 5, 'A');
            display_putc(FONT_PP05, 27, 5, 'u');
            display_putc(FONT_PP05, 23, 5, 'g');
            break;
        case 9:  // Sep
            display_putc(FONT_PP05, 31, 5, 'S');
            display_putc(FONT_PP05, 27, 5, 'e');
            display_putc(FONT_PP05, 23, 5, 'p');
            break;
        case 10:  // Oct
            display_putc(FONT_PP05, 31, 5, 'O');
            display_putc(FONT_PP05, 27, 5, 'c');
            display_putc(FONT_PP05, 23, 5, 't');
            break;
        case 11:  // Nov
            display_putc(FONT_PP05, 31, 5, 'N');
            display_putc(FONT_PP05, 26, 5, 'o');
            display_putc(FONT_PP05, 22, 5, 'v');
            break;
        case 12:  // Dec
            display_putc(FONT_PP05, 31, 5, 'D');
            display_putc(FONT_PP05, 27, 5, 'e');
            display_putc(FONT_PP05, 23, 5, 'c');
            break;
        default:
            display_putc(FONT_PP05, 31, 5, '-');
            display_putc(FONT_PP05, 27, 5, '-');
            display_putc(FONT_PP05, 23, 5, '-');
            break;
        }
    }
    // Draw days
    if (mask & (1 << 9)) {
        if (ct->d >= 10) {
            display_putc(FONT_PP05, 17, 5, '0' + ct->d / 10);
            display_putc(FONT_PP05, 13, 5, '0' + ct->d % 10);
        } else {
            display_putc(FONT_PP05, 15, 5, '0' + ct->d);
        }
    }
    // Draw apostrophe
    if (mask & (1 << 15)) {
        display_putc(FONT_PP05, 8, 5, '\'');
    }
    // Draw years
    if (mask & (1 << 12)) {
        display_putc(FONT_PP05, 6, 5, '0' + ct->yh);
    }
    if (mask & (1 << 11)) {
        display_putc(FONT_PP05, 2, 5, '0' + ct->yl);
    }
    // Draw day-of-week string
    if (mask & (1 << 8)) {
        switch (dow) {
        case DOW_SUNDAY:
            display_putc(FONT_PP05, 31, 11, 'S');
            display_putc(FONT_PP05, 27, 11, 'u');
            display_putc(FONT_PP05, 23, 11, 'n');
            break;
        case DOW_MONDAY:
            display_putc(FONT_PP05, 31, 11, 'M');
            display_putc(FONT_PP05, 25, 11, 'o');
            display_putc(FONT_PP05, 21, 11, 'n');
            break;
        case DOW_TUESDAY:
            display_putc(FONT_PP05, 31, 11, 'T');
            display_putc(FONT_PP05, 27, 11, 'u');
            display_putc(FONT_PP05, 23, 11, 'e');
            break;
        case DOW_WEDNESDAY:
            display_putc(FONT_PP05, 31, 11, 'W');
            display_putc(FONT_PP05, 25, 11, 'e');
            display_putc(FONT_PP05, 21, 11, 'd');
            break;
        case DOW_THURSDAY:
            display_putc(FONT_PP05, 31, 11, 'T');
            display_putc(FONT_PP05, 27, 11, 'h');
            display_putc(FONT_PP05, 23, 11, 'u');
            break;
        case DOW_FRIDAY:
            display_putc(FONT_PP05, 31, 11, 'F');
            display_putc(FONT_PP05, 27, 11, 'r');
            display_putc(FONT_PP05, 24, 11, 'i');
            break;
        case DOW_SATURDAY:
            display_putc(FONT_PP05, 31, 11, 'S');
            display_putc(FONT_PP05, 27, 11, 'a');
            display_putc(FONT_PP05, 23, 11, 't');
            break;
        }
    }
    // Draw colon
    if (mask & (1 << 14)) {
        display_putc(FONT_PP05, 8, 11, ':');
    }
    // Draw hours
    if (mask & (1 << 7)) {
        if (ct->h >= 10) {
            display_putc(FONT_PP05, 16, 11, '0' + ct->h / 10);
            display_putc(FONT_PP05, 12, 11, '0' + ct->h % 10);
        } else {
            display_putc(FONT_PP05, 12, 11, '0' + ct->h);
        }
    }
    // Draw minutes
    if (mask & (1 << 6)) {
        display_putc(FONT_PP05, 6, 11, '0' + ct->m / 10);
    }
    if (mask & (1 << 5)) {
        display_putc(FONT_PP05, 2, 11, '0' + ct->m % 10);
    }
}

// Draw configuration screen RELAY_EVENT_TOP
void draw_config_relay_event_top(uint8_t i, uint8_t count, uint16_t mask) {
    // Bit mask of drawable elements
    //   mask<15:9>  reserved
    //       <8>     relay number
    //       <7>     event count and parentheses
    //       <6:3>   reserved
    //       <2>     caption string "Setup Relay"
    //       <1>     left button icon <down>
    //       <0>     right button icon <next>

    // Draw button icons
    if (mask & (1 << 1)) {
        display_putc(FONT_PP05, 27, 0, '\201');
    }
    if (mask & (1 << 0)) {
        display_putc(FONT_PP05, 11, 0, '\202');
    }
    // Draw caption string "Setup Relay"
    if (mask & (1 << 2)) {
        display_putc(FONT_PP05, 31, 5, 'S');
        display_putc(FONT_PP05, 27, 5, 'e');
        display_putc(FONT_PP05, 23, 5, 't');
        display_putc(FONT_PP05, 20, 5, 'u');
        display_putc(FONT_PP05, 16, 5, 'p');
        display_putc(FONT_PP05, 31, 11, 'R');
        display_putc(FONT_PP05, 27, 11, 'e');
        display_putc(FONT_PP05, 24, 11, 'l');
        display_putc(FONT_PP05, 21, 11, 'a');
        display_putc(FONT_PP05, 17, 11, 'y');
    }
    // Draw relay number
    if (mask & (1 << 8)) {
        display_putc(FONT_PP05, 12, 11, '1' + i);
    }
    // Draw event count and parentheses
    if (mask & (1 << 7)) {
        display_putc(FONT_PP05, 8, 11, '(');
        display_putc(FONT_PP05, 5, 11, '0' + count);
        display_putc(FONT_PP05, 1, 11, ')');
    }
}

// Draw common configuration screen RELAY_EVENT_MOD*
void draw_config_relay_event_mod(uint8_t i, event_t* ev, uint8_t icon_l,
    uint8_t icon_r, uint16_t mask) {
    // Bit mask of drawable elements
    //   mask<15>   event mask indicator dot on Saturdays
    //       <14>   event mask indicator dot on Fridays
    //       <13>   event mask indicator dot on Thursdays
    //       <12>   event mask indicator dot on Wednesdays
    //       <11>   event mask indicator dot on Tuesdays
    //       <10>   event mask indicator dot on Mondays
    //       <9>    event mask indicator dot on Sundays
    //       <8>    event number
    //       <7>    on-event hours
    //       <6>    high digit of on-event minutes
    //       <5>    low digit of on-event minutes
    //       <4>    off-event hours
    //       <3>    high digit of off-event minutes
    //       <2>    low digit of off-event minutes
    //       <1>    left button icon
    //       <0>    right button icon

    // Draw button icons
    if (mask & (1 << 1)) {
        display_putc(FONT_PP05, 27, 0, icon_l);
    }
    if (mask & (1 << 0)) {
        display_putc(FONT_PP05, 11, 0, icon_r);
    }
    // Draw caption string "E"
    display_putc(FONT_PP05, 31, 5, 'E');
    // Draw event number
    if (mask & (1 << 8)) {
        display_putc(FONT_PP05, 27, 5, '1' + i);
    }
    // Draw wavedash
    display_putc(FONT_PP05, 21, 11, '\212');
    // Draw colons
    display_putc(FONT_PP05, 8, 5, ':');
    display_putc(FONT_PP05, 8, 11, ':');
    // Draw on-event hours
    if (mask & (1 << 7)) {
        if (ev->on.h >= 10) {
            display_putc(FONT_PP05, 16, 5, '0' + ev->on.h / 10);
            display_putc(FONT_PP05, 12, 5, '0' + ev->on.h % 10);
        } else {
            display_putc(FONT_PP05, 12, 5, '0' + ev->on.h);
        }
    }
    // Draw on-event minutes
    if (mask & (1 << 6)) {
        display_putc(FONT_PP05, 6, 5, '0' + ev->on.m / 10);
    }
    if (mask & (1 << 5)) {
        display_putc(FONT_PP05, 2, 5, '0' + ev->on.m % 10);
    }
    // Draw off-event hours
    if (mask & (1 << 4)) {
        if (ev->off.h >= 10) {
            display_putc(FONT_PP05, 16, 11, '0' + ev->off.h / 10);
            display_putc(FONT_PP05, 12, 11, '0' + ev->off.h % 10);
        } else {
            display_putc(FONT_PP05, 12, 11, '0' + ev->off.h);
        }
    }
    // Draw off-event minutes
    if (mask & (1 << 3)) {
        display_putc(FONT_PP05, 6, 11, '0' + ev->off.m / 10);
    }
    if (mask & (1 << 2)) {
        display_putc(FONT_PP05, 2, 11, '0' + ev->off.m % 10);
    }
    // Draw event mask indicator dots
    for (uint8_t i = 0; i < 7; i++) {
        if ((mask & (1 << (9 + i))) && (ev->mask & (1 << (1 + i)))) {
            display_putc(FONT_PP05, 31 - i, 11, '\210' + (i & 0x01));
        }
    }
}

// Draw configuration screen RELAY_EVENT_MASK
void draw_config_relay_event_mask(uint8_t i, event_t* ev, uint8_t index_dow,
    uint16_t mask) {
    // Bit mask of drawable elements
    //   mask<15>   event mask indicator dot on Saturdays
    //       <14>   event mask indicator dot on Fridays
    //       <13>   event mask indicator dot on Thursdays
    //       <12>   event mask indicator dot on Wednesdays
    //       <11>   event mask indicator dot on Tuesdays
    //       <10>   event mask indicator dot on Mondays
    //       <9>    event mask indicator dot on Sundays
    //       <8>    event number
    //       <7>    day-of-week string
    //       <6>    ballot box
    //       <5>    value string "on"/"off"
    //       <4:2>  reserved
    //       <1>    left button icon <changevalue>
    //       <0>    right button icon <save>

    bool enabled = ev->mask & (1 << (uint8_t) index_dow);

    // Draw button icons
    if (mask & (1 << 1)) {
        display_putc(FONT_PP05, 27, 0, '\203');
    }
    if (mask & (1 << 0)) {
        display_putc(FONT_PP05, 11, 0, '\205');
    }
    // Draw caption string "E"
    display_putc(FONT_PP05, 31, 5, 'E');
    // Draw event number
    if (mask & (1 << 8)) {
        display_putc(FONT_PP05, 27, 5, '1' + i);
    }
    // Draw day-of-week string
    if (mask & (1 << 8)) {
        switch (index_dow) {
        case DOW_SUNDAY:
            display_putc(FONT_PP05, 18, 5, 'S');
            display_putc(FONT_PP05, 14, 5, 'u');
            display_putc(FONT_PP05, 10, 5, 'n');
            break;
        case DOW_MONDAY:
            display_putc(FONT_PP05, 18, 5, 'M');
            display_putc(FONT_PP05, 12, 5, 'o');
            display_putc(FONT_PP05, 8, 5, 'n');
            break;
        case DOW_TUESDAY:
            display_putc(FONT_PP05, 18, 5, 'T');
            display_putc(FONT_PP05, 14, 5, 'u');
            display_putc(FONT_PP05, 10, 5, 'e');
            break;
        case DOW_WEDNESDAY:
            display_putc(FONT_PP05, 18, 5, 'W');
            display_putc(FONT_PP05, 12, 5, 'e');
            display_putc(FONT_PP05, 8, 5, 'd');
            break;
        case DOW_THURSDAY:
            display_putc(FONT_PP05, 18, 5, 'T');
            display_putc(FONT_PP05, 14, 5, 'h');
            display_putc(FONT_PP05, 10, 5, 'u');
            break;
        case DOW_FRIDAY:
            display_putc(FONT_PP05, 18, 5, 'F');
            display_putc(FONT_PP05, 14, 5, 'r');
            display_putc(FONT_PP05, 11, 5, 'i');
            break;
        case DOW_SATURDAY:
            display_putc(FONT_PP05, 18, 5, 'S');
            display_putc(FONT_PP05, 14, 5, 'a');
            display_putc(FONT_PP05, 10, 5, 't');
            break;
        }
    }
    // Draw ballot box
    if (mask & (1 << 6)) {
        display_putc(FONT_PP05, 16, 11, enabled ? '\207' : '\206');
    }
    // Draw value string "on"/"off"
    if (mask & (1 << 5)) {
        if (enabled) {
            display_putc(FONT_PP05, 8, 11, 'o');
            display_putc(FONT_PP05, 4, 11, 'n');
        } else {
            display_putc(FONT_PP05, 9, 11, 'o');
            display_putc(FONT_PP05, 5, 11, 'f');
            display_putc(FONT_PP05, 2, 11, 'f');
        }
    }
    // Draw event mask indicator dots
    for (uint8_t i = 0; i < 7; i++) {
        if ((mask & (1 << (9 + i))) && (ev->mask & (1 << (1 + i)))) {
            display_putc(FONT_PP05, 31 - i, 11, '\210' + (i & 0x01));
        }
    }
}

// Draw configuration screen BRIGHTNESS
void draw_config_brightness(uint8_t br, uint16_t mask) {
    // Bit mask of drawable elements
    //   mask<15:7>  reserved
    //       <6>     bar image
    //       <5>     value string "auto"/"1"/"2"/"3"/"4"
    //       <4:3>   reserved
    //       <2>     condensed caption string "Brightness"
    //       <1>     left button icon <changevalue>
    //       <0>     right button icon <save>

    uint8_t img0 = ' ';
    uint8_t img1 = ' ';

    // Draw button icons
    if (mask & (1 << 1)) {
        display_putc(FONT_PP05, 27, 0, '\203');
    }
    if (mask & (1 << 0)) {
        display_putc(FONT_PP05, 11, 0, '\205');
    }
    // Draw condensed caption string "Brightness"
    if (mask & (1 << 2)) {
        display_putc(FONT_PP05, 31, 5, '\213');
        display_putc(FONT_PP05, 23, 5, '\214');
        display_putc(FONT_PP05, 15, 5, '\215');
        display_putc(FONT_PP05, 7, 5, '\216');
    }
    // Draw bar image
    if (mask & (1 << 6)) {
        switch (br) {
        case 1:
            img0 = '\221';
            img1 = '\222';
            break;
        case 2:
            img0 = '\223';
            img1 = '\224';
            break;
        case 3:
            img0 = '\223';
            img1 = '\225';
            break;
        case 4:
            img0 = '\223';
            img1 = '\223';
            break;
        default:
            img0 = '\217';
            img1 = '\220';
            break;
        }
        display_putc(FONT_PP05, 31, 11, img0);
        display_putc(FONT_PP05, 23, 11, img1);
    }
    // Draw value string "auto"/"1"/"2"/"3"/"4"
    if (mask & (1 << 5)) {
        switch (br) {
        case 1:
            display_putc(FONT_PP05, 11, 11, '2');
            display_putc(FONT_PP05, 7, 11, '5');
            display_putc(FONT_PP05, 3, 11, '%');
            break;
        case 2:
            display_putc(FONT_PP05, 11, 11, '5');
            display_putc(FONT_PP05, 7, 11, '0');
            display_putc(FONT_PP05, 3, 11, '%');
            break;
        case 3:
            display_putc(FONT_PP05, 11, 11, '7');
            display_putc(FONT_PP05, 7, 11, '5');
            display_putc(FONT_PP05, 3, 11, '%');
            break;
        case 4:
            display_putc(FONT_PP05, 14, 11, '1');
            display_putc(FONT_PP05, 11, 11, '0');
            display_putc(FONT_PP05, 7, 11, '0');
            display_putc(FONT_PP05, 3, 11, '%');
            break;
        default:
            display_putc(FONT_PP05, 13, 11, 'a');
            display_putc(FONT_PP05, 9, 11, 'u');
            display_putc(FONT_PP05, 5, 11, 't');
            display_putc(FONT_PP05, 2, 11, 'o');
            break;
        }
    }
}

// Draw configuration screen SAVE_CONFIRM
void draw_config_save_confirm(bool save_to_ee, uint16_t mask) {
    // Bit mask of drawable elements
    //   mask<15:7>  reserved
    //       <6>     ballot box
    //       <5>     value string "yes"/"no"
    //       <4:3>   reserved
    //       <2>     caption string "Save to EE"
    //       <1>     left button icon <changevalue>
    //       <0>     right button icon <save>

    // Draw button icons
    if (mask & (1 << 1)) {
        display_putc(FONT_PP05, 27, 0, '\203');
    }
    if (mask & (1 << 0)) {
        display_putc(FONT_PP05, 11, 0, '\205');
    }
    // Draw caption string "Save to EE"
    if (mask & (1 << 2)) {
        display_putc(FONT_PP05, 31, 5, 'S');
        display_putc(FONT_PP05, 27, 5, 'a');
        display_putc(FONT_PP05, 23, 5, 'v');
        display_putc(FONT_PP05, 19, 5, 'e');
        display_putc(FONT_PP05, 14, 5, 't');
        display_putc(FONT_PP05, 11, 5, 'o');
        display_putc(FONT_PP05, 6, 5, 'E');
        display_putc(FONT_PP05, 2, 5, 'E');
    }
    // Draw ballot box
    if (mask & (1 << 6)) {
        display_putc(FONT_PP05, 17, 11, save_to_ee ? '\207' : '\206');
    }
    // Draw value string "yes"/"no"
    if (mask & (1 << 5)) {
        if (save_to_ee) {
            display_putc(FONT_PP05, 10, 11, 'y');
            display_putc(FONT_PP05, 6, 11, 'e');
            display_putc(FONT_PP05, 2, 11, 's');
        } else {
            display_putc(FONT_PP05, 9, 11, 'n');
            display_putc(FONT_PP05, 5, 11, 'o');
        }
    }
}

// Draw ADC value of light sensor
void draw_light_adc(uint16_t adc) {
    // Draw caption string "Light sen."
    display_putc(FONT_PP05, 31, 0, 'L');
    display_putc(FONT_PP05, 27, 0, 'i');
    display_putc(FONT_PP05, 25, 0, 'g');
    display_putc(FONT_PP05, 21, 0, 'h');
    display_putc(FONT_PP05, 17, 0, 't');
    display_putc(FONT_PP05, 13, 0, 's');
    display_putc(FONT_PP05, 9, 0, 'e');
    display_putc(FONT_PP05, 5, 0, 'n');
    display_putc(FONT_PP05, 1, 0, '.');
    // Draw caption string "ADC"
    display_putc(FONT_PP05, 31, 11, 'A');
    display_putc(FONT_PP05, 27, 11, 'D');
    display_putc(FONT_PP05, 23, 11, 'C');
    // Draw ADC value
    if (adc >= 1000) {
        display_putc(FONT_M0410, 18, 6, '0' + adc / 1000 % 10);
    }
    if (adc >= 100) {
        display_putc(FONT_M0410, 13, 6, '0' + adc / 100 % 10);
    }
    if (adc >= 10) {
        display_putc(FONT_M0410, 8, 6, '0' + adc / 10 % 10);
    }
    display_putc(FONT_M0410, 3, 6, '0' + adc % 10);
}
