/*
 * DotMatrixClock2018/light_sensor.c
 *
 *  Author: kayekss
 *  Target: ATmega328P, 20.000 MHz crystal oscillator
 */

#include <stdint.h>
#include "adc.h"
#include "light_sensor.h"

uint8_t adc_to_brightness_level(uint16_t adc) {
    if (adc <= BRIGHTNESS_THRESHOLD34) {
        return 4;
    } else if (adc <= BRIGHTNESS_THRESHOLD23) {
        return 3;
    } else if (adc <= BRIGHTNESS_THRESHOLD12) {
        return 2;
    } else if (adc <= BRIGHTNESS_THRESHOLD01) {
        return 1;
    } else {
        return 0;
    }
}
