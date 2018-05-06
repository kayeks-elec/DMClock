/*
 * DotMatrixClock2018/drawings.h
 *
 *  Author: kayekss
 *  Target: ATmega328P, 20.000 MHz crystal oscillator
 */

#ifndef DRAWINGS_H_
#define DRAWINGS_H_

// Clock digit
typedef struct {
    // Character code for current digit
    uint8_t chr;
    // Character code for scrolling
    uint8_t chr_scroll;
    // Scrolling frame count (0..(font height + 1))
    uint8_t frame;
} cdigit_t;

void draw_time_hm(ctime_t* ct, uint16_t mask);
void draw_time_hms(ctime_t* ct, uint16_t mask);
void draw_date_dayofweek(ctime_t* ct, dow_t dow, uint16_t mask);
void draw_date_year(ctime_t* ct, uint16_t mask);
void draw_temperature(bool result, uint8_t sign, uint8_t integer,
    uint16_t fraction_x10k, uint16_t mask);
void draw_gps_status(uint8_t fix, uint8_t blink_phase, uint16_t mask);
void draw_config_use_gps(bool use_gps, uint16_t mask);
void draw_config_set_time_top(uint16_t mask);
void draw_config_set_time_mod(ctime_t* ct, dow_t dow, uint8_t icon_l,
    uint8_t icon_r, uint16_t mask);
void draw_config_relay_event_top(uint8_t i, uint8_t count, uint16_t mask);
void draw_config_relay_event_mod(uint8_t i, event_t* ev, uint8_t icon_l,
    uint8_t icon_r, uint16_t mask);
void draw_config_relay_event_mask(uint8_t i, event_t* ev, uint8_t index_dow,
    uint16_t mask);
void draw_config_brightness(uint8_t br, uint16_t mask);
void draw_config_save_confirm(bool save_to_ee, uint16_t mask);
void draw_light_adc(uint16_t adc);

#endif
