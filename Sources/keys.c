/*
 * DotMatrixClock2018/keys.c
 *
 *  Author: kayekss
 *  Target: ATmega328P, 20.000 MHz crystal oscillator
 */

#include <stdint.h>
#include "keys.h"

// Initialize key watcher
void key_initialize(key_t* k) {
    k->pressed = 0;
    k->released = 0;
}

// Poll key input
void key_poll(key_t* k, uint8_t level) {
#ifdef KEY_INPUT_LOW_ACTIVE
    if (!level)
#elif
    if (level)
#endif
    {
        // Key is pressed
        k->released = 0;
        if (k->pressed != UINT8_MAX) {
            k->pressed++;
    }
        } else {
        // Key is released
        k->released = k->pressed;
        k->pressed = 0;
    }
}

// Check if the key is just pressed on last polling
uint8_t key_is_pressed(key_t* k) {
    return k->pressed == 1;
}

// Check if the key is just released on last polling
uint8_t key_is_released(key_t* k) {
    return k->released > 0;
}

// Check if the key is kept pressed for the specified poll times
uint8_t key_is_holded(key_t* k, uint8_t polls) {
    return k->pressed >= polls;
}
