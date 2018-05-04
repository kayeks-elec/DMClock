/*
 * adc.c
 *
 *  Author: kayekss
 *  Target: ATmega328P
 */

#include <stdint.h>
#include <avr/io.h>
#include "adc.h"

// Read ADC channel (blocking operation); return 10-bit result
uint16_t read_adc(uint8_t adcsel) {
    // Set input channel
    ADMUX = (ADMUX & 0xf0) | (adcsel & 0x0f);
    
    // Wait until ADC is ready
    while (ADCSRA & (1 << ADSC));
    // Start conversion
    ADCSRA |= (1 << ADSC);
    // Wait until ADC finishes conversion
    while (ADCSRA & (1 << ADSC));
    return ADC & 0x03ffu;
}
