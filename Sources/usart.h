/*
 * usart.h
 *
 *  Author: kayekss
 *  Target: ATmega328P
 */

#ifndef USART_H_
#define USART_H_

// Line buffer structure
typedef struct {
    // Length of buffer data
    uint8_t length;
    // Data bytes currently stored
    volatile uint8_t count;
    // Buffer data entity
    uint8_t *data;
} linebuf_t;

// Ring buffer structure
typedef struct {
    // Length of buffer data
    uint8_t length;
    // Read pointer
    uint8_t rp;
    // Write pointer
    uint8_t wp;
    // Buffer full flag (0: empty, 1: full when rp equals to wp)
    bool full;
    // Buffer data entity
    uint8_t *data;
} ringbuf_t;

void usart_putc(uint8_t d);
void usart_puts(char* s);
void linebuf_allocate(linebuf_t* b, uint16_t length);
void linebuf_deallocate(linebuf_t* b);
void linebuf_initialize(linebuf_t* b, uint16_t length);
bool linebuf_available(linebuf_t* b);
void linebuf_clear(linebuf_t* b);
uint8_t linebuf_put(linebuf_t* b, uint8_t c);
void ringbuf_allocate(ringbuf_t* b, uint16_t length);
void ringbuf_deallocate(ringbuf_t* b);
void ringbuf_initialize(ringbuf_t* b, uint16_t length);
bool ringbuf_available(ringbuf_t* b);
void ringbuf_clear(ringbuf_t* b);
uint8_t ringbuf_put(ringbuf_t* b, uint8_t c);
uint8_t ringbuf_get(ringbuf_t* b, uint8_t* c);

#endif
