/*
 * DotMatrixClock2018/keys.h
 *
 *  Author: kayekss
 *  Target: ATmega328P, 20.000 MHz crystal oscillator
 */

#ifndef KEYS_H_
#define KEYS_H_

#define KEY_INPUT_LOW_ACTIVE

typedef struct {
    uint8_t pressed;
    uint8_t released;
} key_t;

void key_initialize(key_t* k);
void key_poll(key_t* k, uint8_t level);
uint8_t key_is_pressed(key_t* k);
uint8_t key_is_released(key_t* k);
uint8_t key_is_holded(key_t* k, uint8_t polls);

#endif
