/*
 * DotMatrixClock2018/light_sensor.h
 *
 *  Author: kayekss
 *  Target: ATmega328P, 20.000 MHz crystal oscillator
 */

#ifndef LIGHT_SENSOR_H_
#define LIGHT_SENSOR_H_

// Thresholds of ADC value for brightness adjustment
#define BRIGHTNESS_THRESHOLD0  1024  // Level    1 => 0
#define BRIGHTNESS_THRESHOLD1   900  // Level 0, 2 => 1
#define BRIGHTNESS_THRESHOLD2   800  // Level 1, 3 => 2
#define BRIGHTNESS_THRESHOLD3   600  // Level 2, 4 => 3
#define BRIGHTNESS_THRESHOLD4   300  // Level 3    => 4

uint8_t adc_to_brightness_level(uint16_t adc, uint8_t c_level);

#endif
