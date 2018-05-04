/*
 * usart.c
 *
 *  Author: kayekss
 *  Target: ATmega328P
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <avr/io.h>
#include "usart.h"

// Send a byte over USART
inline void usart_putc(uint8_t d) {
    // Wait until transmit register is ready
    while (!(UCSR0A & (1 << UDRE0)));
    // Clear transmission complete flag
    UCSR0A |= (1 << TXC0);
    // Set data byte
    UDR0 = d;
}

// Send a string over USART
void usart_puts(char* s) {
    while (*s) {
        usart_putc(*s++);
    }
}

// Allocate memory for buffer entity
void linebuf_allocate(linebuf_t* b, uint16_t length) {
    b->data = calloc(length, sizeof(uint8_t));
    b->length = b->data ? length : 0;
}

// Deallocate memory for buffer entity
void linebuf_deallocate(linebuf_t* b) {
    free(b->data);    
}

// Initialize line buffer structure
void linebuf_initialize(linebuf_t* b, uint16_t length) {
    b->length = 0;
    b->data = NULL;
    linebuf_allocate(b, length);
    b->count = 0;
}

// Check if any data is available
inline bool linebuf_available(linebuf_t* b) {
    return b->count > 0;
}

// Clear all data
inline void linebuf_clear(linebuf_t* b) {
    b->count = 0;
}

// Put a data byte into the buffer
// ... return zero on success, non-zero on failure
inline uint8_t linebuf_put(linebuf_t* b, uint8_t c) {
    if (b->count == b->length) {
        return 1;
    } else {
        b->data[b->count++] = c;
        return 0;
    }
}

// Allocate memory for buffer entity
void ringbuf_allocate(ringbuf_t* b, uint16_t length) {
    b->data = calloc(length, sizeof(uint8_t));
    b->length = length;
}

// Deallocate memory for buffer entity
void ringbuf_deallocate(ringbuf_t* b) {
    free(b->data);
}

// Initialize ring buffer structure
void ringbuf_initialize(ringbuf_t* b, uint16_t length) {
    b->rp = 0;
    b->wp = 0;
    b->full = false;
    b->data = NULL;
    ringbuf_allocate(b, length);
}

// Check if any data is available
inline bool ringbuf_available(ringbuf_t* b) {
    return b->full || b->rp != b->wp;
}

// Clear all data
inline void ringbuf_clear(ringbuf_t* b) {
    b->wp = b->rp;
    b->full = false;
}

// Put a data byte into the buffer
// ... return zero on success, non-zero on failure
inline uint8_t ringbuf_put(ringbuf_t* b, uint8_t c) {
    uint8_t result = 0;
    
    if (b->full) {
        result = 1;
    } else {
        b->data[b->wp] = c;
        // Advance write pointer
        b->wp = b->wp >= b->length - 1 ? 0 : b->wp + 1;
        // Check if buffer is full
        b->full = b->wp == b->rp;
        result = 0;
    }
    return result;
}

// Get a data byte from the buffer
// ... return zero on success, non-zero on failure
inline uint8_t ringbuf_get(ringbuf_t* b, uint8_t* c) {
    uint8_t result = 0;
    
    if (ringbuf_available(b)) {
        *c = b->data[b->rp];
        // Advance read pointer
        b->rp = b->rp >= b->length - 1 ? 0 : b->rp + 1; 
        // Clear full flag
        b->full = false;
        result = 0;
    } else {
        result = 1;
    }
    return result;
}
