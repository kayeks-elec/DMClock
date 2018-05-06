/*
 * DotMatrixClock2018/light_sensor.c
 *
 *  Author: kayekss
 *  Target: ATmega328P, 20.000 MHz crystal oscillator
 */

#include <stdint.h>
#include "light_sensor.h"

// Get brightness level by ADC result, with hysteresis transition
uint8_t adc_to_brightness_level(uint16_t adc, uint8_t c_level) {
    uint8_t result = 0;
    uint16_t const lut_brightness_threshold[] = {
        BRIGHTNESS_THRESHOLD0,
        BRIGHTNESS_THRESHOLD1,
        BRIGHTNESS_THRESHOLD2,
        BRIGHTNESS_THRESHOLD3,
        BRIGHTNESS_THRESHOLD4
    };
    
    if (c_level != 0 && adc >= lut_brightness_threshold[c_level - 1]) {
        result = c_level - 1;
    } else if (c_level != 4 && adc <= lut_brightness_threshold[c_level + 1]) {
        result = c_level + 1;
    } else {
        result = c_level;
    }
    return result;
}
