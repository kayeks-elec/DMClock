/*
 * DotMatrixClock2018/light_sensor.h
 *
 *  Author: kayekss
 *  Target: ATmega328P, 20.000 MHz crystal oscillator
 */

#ifndef LIGHT_SENSOR_H_
#define LIGHT_SENSOR_H_

// Thresholds of ADC value for brightness adjustment
#define BRIGHTNESS_THRESHOLD01  1024  // Level 0 <=> 1
#define BRIGHTNESS_THRESHOLD12   900  // Level 1 <=> 2
#define BRIGHTNESS_THRESHOLD23   800  // Level 2 <=> 3
#define BRIGHTNESS_THRESHOLD34   300  // Level 3 <=> 4

uint8_t adc_to_brightness_level(uint16_t adc);

#endif
