/*
 * DotMatrixClock2018/display.h
 *
 *  Author: kayekss
 *  Target: ATmega328P, 20.000 MHz crystal oscillator
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

// Font definitions
typedef enum {
    // Monospace, width 4, height 10
    FONT_M0410 = 0x14,
    // Monospace, width 6, height 10
    FONT_M0610 = 0x16,
    // Proportional, various width, height 5
    FONT_PP05  = 0x05
} font_t;
enum {
    FONT_PROPORTIONAL = 0x00,
    FONT_MONOSPACED = 0x10
};
enum {
    FONT_MASK = 0xf0
};

void display_clear();
void display_sync();
void display_putc(font_t f, uint8_t x, uint8_t y, uint8_t c);
void display_putc_scroll(font_t f, uint8_t x, uint8_t y, uint8_t c_ex,
    uint8_t c_new, uint8_t frame);

#endif
