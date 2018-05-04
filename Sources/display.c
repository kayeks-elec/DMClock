/*
 * DotMatrixClock2018/display.c
 *
 *  Author: kayekss
 *  Target: ATmega328P, 20.000 MHz crystal oscillator
 */

#include <stdlib.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "display.h"

// Front frame buffer - to be read from display ISR
volatile uint32_t fb_front[16];
// Back frame buffer - drawing functions should write to this plane
volatile uint32_t fb_back[16];

// Index table for font "M0410"
PROGMEM uint16_t const font_index_m0410[128] = {
    // 0x00
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
    // 0x10
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
    // 0x20
     10,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,  20,  30,   0,
    // 0x30
     40,  50,  60,  70,  80,  90, 100, 110,
    120, 130, 140,   0,   0,   0,   0,   0,
    // 0x40
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
    // 0x50
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
    // 0x60
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
    // 0x70
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0
};
// Bitmap for font "M0410"
PROGMEM uint8_t const font_bitmap_m0410[] = {
    // (+  0) Unimplemented character
    0x0a, 0x05, 0x0a, 0x05, 0x0a, 0x05, 0x0a, 0x05, 0x0a, 0x05,
    // (+ 10) Space
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // (+ 20) -
    0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x07, 0x00, 0x00, 0x00,
    // (+ 30) .
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x0c,
    // (+ 40) 0
    0x06, 0x0f, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0f, 0x06,
    // (+ 50) 1
    0x0e, 0x0e, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x0f, 0x0f,
    // (+ 60) 2
    0x0e, 0x0f, 0x03, 0x03, 0x03, 0x06, 0x0c, 0x08, 0x0f, 0x0f,
    // (+ 70) 3
    0x0f, 0x0f, 0x03, 0x06, 0x07, 0x03, 0x03, 0x03, 0x0f, 0x0e,
    // (+ 80) 4
    0x03, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0f, 0x0f, 0x03, 0x03,
    // (+ 90) 5
    0x0f, 0x0f, 0x08, 0x0e, 0x0f, 0x03, 0x03, 0x03, 0x0f, 0x0e,
    // (+100) 6
    0x07, 0x0f, 0x08, 0x0e, 0x0f, 0x0b, 0x0b, 0x0b, 0x0f, 0x06,
    // (+110) 7
    0x0f, 0x0f, 0x03, 0x03, 0x06, 0x06, 0x06, 0x0c, 0x0c, 0x0c,
    // (+120) 8
    0x06, 0x0f, 0x0d, 0x0d, 0x0e, 0x07, 0x0b, 0x0b, 0x0f, 0x06,
    // (+130) 9
    0x06, 0x0f, 0x0b, 0x0b, 0x0b, 0x0f, 0x07, 0x03, 0x0f, 0x0e,
    // (+140) :
    0x00, 0x00, 0x00, 0x0c, 0x0c, 0x00, 0x00, 0x0c, 0x0c, 0x00
};

// Index table for font "M0610"
PROGMEM uint16_t const font_index_m0610[128] = {
    // 0x00
      0,   0,   0, 130,   0,   0, 140,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
    // 0x10
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
    // 0x20
     10,   0,   0,   0,   0,  20,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
    // 0x30
     30,  40,  50,  60,  70,  80,  90, 100,
    110, 120,   0,   0,   0,   0,   0,   0,
    // 0x40
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
    // 0x50
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
    // 0x60
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
    // 0x70
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0
};
// Bitmap for font "M0610"
PROGMEM uint8_t const font_bitmap_m0610[] = {
    // (+  0) Unimplemented character
    0x2a, 0x15, 0x2a, 0x15, 0x2a, 0x15, 0x2a, 0x15, 0x2a, 0x15,
    // (+ 10) Space
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // (+ 20) %
    0x00, 0x00, 0x38, 0x2b, 0x37, 0x0e, 0x1c, 0x3b, 0x35, 0x37,
    // (+ 30) 0
    0x1e, 0x3f, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3f, 0x1e,
    // (+ 40) 1
    0x1c, 0x1c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x1e, 0x1e,
    // (+ 50) 2
    0x3e, 0x3f, 0x03, 0x03, 0x07, 0x0e, 0x1c, 0x38, 0x3f, 0x3f,
    // (+ 60) 3
    0x3f, 0x3f, 0x03, 0x07, 0x0e, 0x0f, 0x03, 0x03, 0x3f, 0x3e,
    // (+ 70) 4
    0x03, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3f, 0x3f, 0x03, 0x03,
    // (+ 80) 5
    0x3f, 0x3f, 0x30, 0x3e, 0x3f, 0x03, 0x03, 0x03, 0x3f, 0x3e,
    // (+ 90) 6
    0x1e, 0x3e, 0x30, 0x3e, 0x3f, 0x33, 0x33, 0x33, 0x3f, 0x1e,
    // (+100) 7
    0x3f, 0x3f, 0x03, 0x03, 0x06, 0x06, 0x0c, 0x0c, 0x18, 0x18,
    // (+110) 8
    0x1e, 0x3f, 0x33, 0x33, 0x1e, 0x3f, 0x33, 0x33, 0x3f, 0x1e,
    // (+120) 9
    0x1e, 0x3f, 0x33, 0x33, 0x33, 0x3f, 0x1f, 0x03, 0x1f, 0x1e,
    // (+130) Degree celsius
    0x00, 0x38, 0x2f, 0x3f, 0x18, 0x18, 0x18, 0x18, 0x1f, 0x0f,
    // (+140) Degree fahrenheit
    0x00, 0x38, 0x2f, 0x3f, 0x18, 0x1e, 0x1e, 0x18, 0x18, 0x18
};

// Index table for font "PP05"
PROGMEM uint16_t const font_index_pp05[160] = {
    // 0x00
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
    // 0x10
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
    // 0x20
      6,  12,  18,  24,  30,  36,  42,  48,
     54,  60,  66,  72,  78,  84,  90,  96,
    // 0x30
    102, 108, 114, 120, 126, 132, 138, 144,
    150, 156, 162, 168, 174, 180, 186, 192,
    // 0x40
    198, 204, 210, 216, 222, 228, 234, 240,
    246, 252, 258, 264, 270, 276, 282, 288,
    // 0x50
    294, 300, 306, 312, 318, 324, 330, 336,
    342, 348, 354, 360, 366, 372, 378, 384,
    // 0x60
    390, 396, 402, 408, 414, 420, 426, 432,
    438, 444, 450, 456, 462, 468, 474, 480,
    // 0x70
    486, 492, 498, 504, 510, 516, 522, 528,
    534, 540, 546, 552, 558, 564, 570, 576,
    // 0x80
    582, 588, 594, 600, 606, 612, 618, 624,
    630, 636, 642, 648, 654, 660, 666, 672,
    // 0x90
    678, 684, 690, 696, 702, 708,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0
};
// Bitmap for font "PP05"
// (each character has 1 width byte on its head)
PROGMEM uint8_t const font_wbitmap_pp05[] = {
    // (+  0) Unimplemented character
    3, 0x05, 0x02, 0x05, 0x02, 0x05,
    // (+  6) space
    3, 0x00, 0x00, 0x00, 0x00, 0x00,
    // (+ 12) !
    1, 0x01, 0x01, 0x01, 0x00, 0x01,
    // (+ 18) "
    3, 0x05, 0x05, 0x00, 0x00, 0x00,
    // (+ 24) #
    4, 0x05, 0x0f, 0x05, 0x0f, 0x05,
    // (+ 30) $
    3, 0x07, 0x06, 0x07, 0x03, 0x07,
    // (+ 36) %
    4, 0x00, 0x0d, 0x0a, 0x05, 0x0b,
    // (+ 42) &
    4, 0x0e, 0x08, 0x07, 0x0a, 0x0f,
    // (+ 48) '
    1, 0x01, 0x01, 0x00, 0x00, 0x00,
    // (+ 54) (
    2, 0x01, 0x02, 0x02, 0x02, 0x01,
    // (+ 60) )
    2, 0x02, 0x01, 0x01, 0x01, 0x02,
    // (+ 66) *
    3, 0x00, 0x05, 0x02, 0x05, 0x00,
    // (+ 72) +
    3, 0x00, 0x02, 0x07, 0x02, 0x00,
    // (+ 78) ,
    2, 0x00, 0x00, 0x00, 0x01, 0x02,
    // (+ 84) -
    3, 0x00, 0x00, 0x07, 0x00, 0x00,
    // (+ 90) .
    1, 0x00, 0x00, 0x00, 0x00, 0x01,
    // (+ 96) /
    2, 0x00, 0x01, 0x01, 0x02, 0x02,
    // (+102) 0
    3, 0x07, 0x05, 0x05, 0x05, 0x07,
    // (+108) 1
    3, 0x06, 0x02, 0x02, 0x02, 0x07,
    // (+114) 2
    3, 0x07, 0x01, 0x07, 0x04, 0x07,
    // (+120) 3
    3, 0x07, 0x01, 0x06, 0x01, 0x07,
    // (+126) 4
    3, 0x01, 0x05, 0x05, 0x07, 0x01,
    // (+132) 5
    3, 0x07, 0x04, 0x07, 0x01, 0x07,
    // (+138) 6
    3, 0x07, 0x04, 0x07, 0x05, 0x07,
    // (+144) 7
    3, 0x07, 0x01, 0x01, 0x02, 0x02,
    // (+150) 8
    3, 0x07, 0x05, 0x07, 0x05, 0x07,
    // (+156) 9
    3, 0x07, 0x05, 0x07, 0x01, 0x07,
    // (+162) :
    1, 0x00, 0x01, 0x00, 0x00, 0x01,
    // (+168) ;
    2, 0x00, 0x01, 0x00, 0x01, 0x02,
    // (+174) <
    3, 0x01, 0x02, 0x04, 0x02, 0x01,
    // (+180) =
    3, 0x00, 0x07, 0x00, 0x07, 0x00,
    // (+186) >
    3, 0x04, 0x02, 0x01, 0x02, 0x04,
    // (+192) ?
    3, 0x07, 0x01, 0x03, 0x00, 0x02,
    // (+198) @
    4, 0x07, 0x09, 0x0b, 0x08, 0x07,
    // (+204) A
    3, 0x07, 0x05, 0x05, 0x07, 0x05,
    // (+210) B
    3, 0x07, 0x05, 0x06, 0x05, 0x07,
    // (+216) C
    3, 0x07, 0x04, 0x04, 0x04, 0x07,
    // (+222) D
    3, 0x06, 0x05, 0x05, 0x05, 0x06,
    // (+228) E
    3, 0x07, 0x04, 0x07, 0x04, 0x07,
    // (+234) F
    3, 0x07, 0x04, 0x07, 0x04, 0x04,
    // (+240) G
    3, 0x07, 0x04, 0x05, 0x05, 0x07,
    // (+246) H
    3, 0x05, 0x05, 0x07, 0x05, 0x05,
    // (+252) I
    3, 0x07, 0x02, 0x02, 0x02, 0x07,
    // (+258) J
    3, 0x03, 0x01, 0x01, 0x01, 0x06,
    // (+264) K
    3, 0x05, 0x05, 0x06, 0x05, 0x05,
    // (+270) L
    3, 0x04, 0x04, 0x04, 0x04, 0x07,
    // (+276) M
    5, 0x11, 0x1b, 0x15, 0x11, 0x11,
    // (+282) N
    4, 0x09, 0x0d, 0x0b, 0x09, 0x09,
    // (+288) O
    3, 0x07, 0x05, 0x05, 0x05, 0x07,
    // (+294) P
    3, 0x07, 0x05, 0x05, 0x07, 0x04,
    // (+300) Q
    3, 0x07, 0x05, 0x05, 0x06, 0x03,
    // (+306) R
    3, 0x07, 0x05, 0x05, 0x06, 0x05,
    // (+312) S
    3, 0x07, 0x04, 0x02, 0x01, 0x07,
    // (+318) T
    3, 0x07, 0x02, 0x02, 0x02, 0x02,
    // (+324) U
    3, 0x05, 0x05, 0x05, 0x05, 0x07,
    // (+330) V
    3, 0x05, 0x05, 0x05, 0x05, 0x06,
    // (+336) W
    5, 0x11, 0x15, 0x15, 0x15, 0x1e,
    // (+342) X
    3, 0x05, 0x05, 0x02, 0x05, 0x05,
    // (+348) Y
    3, 0x05, 0x05, 0x07, 0x02, 0x02,
    // (+354) Z
    3, 0x07, 0x01, 0x02, 0x04, 0x07,
    // (+360) [
    2, 0x03, 0x02, 0x02, 0x02, 0x03,
    // (+366) \ (Backslash)
    2, 0x00, 0x02, 0x02, 0x01, 0x01,
    // (+372) ]
    2, 0x03, 0x01, 0x01, 0x01, 0x03,
    // (+378) ^
    3, 0x02, 0x05, 0x00, 0x00, 0x00,
    // (+384) _
    3, 0x00, 0x00, 0x00, 0x00, 0x07,
    // (+390) `
    3, 0x02, 0x01, 0x00, 0x00, 0x00,
    // (+396) a
    3, 0x00, 0x06, 0x03, 0x05, 0x07,
    // (+402) b
    3, 0x04, 0x07, 0x05, 0x05, 0x07,
    // (+408) c
    3, 0x00, 0x07, 0x04, 0x04, 0x07,
    // (+414) d
    3, 0x01, 0x07, 0x05, 0x05, 0x07,
    // (+420) e
    3, 0x00, 0x03, 0x05, 0x06, 0x03,
    // (+426) f
    2, 0x03, 0x02, 0x03, 0x02, 0x02,
    // (+432) g
    3, 0x00, 0x03, 0x05, 0x03, 0x06,
    // (+438) h
    3, 0x04, 0x07, 0x05, 0x05, 0x05,
    // (+444) i
    1, 0x01, 0x00, 0x01, 0x01, 0x01,
    // (+450) j
    2, 0x01, 0x00, 0x01, 0x01, 0x02,
    // (+456) k
    3, 0x04, 0x05, 0x06, 0x05, 0x05,
    // (+462) l
    2, 0x03, 0x01, 0x01, 0x01, 0x01,
    // (+468) m
    5, 0x00, 0x1e, 0x15, 0x15, 0x15,
    // (+474) n
    3, 0x00, 0x06, 0x05, 0x05, 0x05,
    // (+480) o
    3, 0x00, 0x07, 0x05, 0x05, 0x07,
    // (+486) p
    3, 0x00, 0x07, 0x05, 0x07, 0x04,
    // (+492) q
    3, 0x00, 0x07, 0x05, 0x07, 0x01,
    // (+498) r
    2, 0x00, 0x03, 0x02, 0x02, 0x02,
    // (+504) s
    3, 0x00, 0x03, 0x06, 0x01, 0x07,
    // (+510) t
    2, 0x02, 0x03, 0x02, 0x02, 0x03,
    // (+516) u
    3, 0x00, 0x05, 0x05, 0x05, 0x07,
    // (+522) v
    3, 0x00, 0x05, 0x05, 0x05, 0x06,
    // (+528) w
    5, 0x00, 0x15, 0x15, 0x15, 0x1e,
    // (+534) x
    3, 0x00, 0x05, 0x02, 0x05, 0x05,
    // (+540) y
    3, 0x00, 0x05, 0x05, 0x03, 0x06,
    // (+546) z
    3, 0x00, 0x07, 0x02, 0x04, 0x07,
    // (+552) {
    3, 0x03, 0x02, 0x06, 0x02, 0x03,
    // (+558) |
    1, 0x01, 0x01, 0x01, 0x01, 0x01,
    // (+564) }
    3, 0x06, 0x02, 0x03, 0x02, 0x06,
    // (+570) ~
    3, 0x07, 0x00, 0x00, 0x00, 0x00,
    // (+576) \177 Degree
    3, 0x07, 0x05, 0x07, 0x00, 0x00,
    // (+582) \200 Button icon <up>
    8, 0x08, 0x1c, 0x3e, 0x00, 0x00,
    // (+588) \201 Button icon <down>
    8, 0x00, 0x3e, 0x1c, 0x08, 0x00,
    // (+594) \202 Button icon <next>
    8, 0x24, 0x36, 0x36, 0x24, 0x00,
    // (+600) \203 Button icon <changevalue>
    8, 0x00, 0x04, 0x6e, 0x04, 0x00,
    // (+606) \204 Button icon <discard>
    8, 0x24, 0x18, 0x18, 0x24, 0x00,
    // (+612) \205 Button icon <save>
    8, 0x04, 0x2c, 0x38, 0x10, 0x00,
    // (+618) \206 Ballot box
    5, 0x1f, 0x11, 0x11, 0x11, 0x1f,
    // (+624) \207 Ballot box with check
    5, 0x1f, 0x13, 0x15, 0x19, 0x1f,
    // (+630) \210 Lower single dot
    1, 0x00, 0x00, 0x00, 0x01, 0x00,
    // (+636) \211 Higher single dot
    1, 0x00, 0x01, 0x00, 0x00, 0x00,
    // (+642) \212 Wavedash
    4, 0x00, 0x00, 0x0d, 0x0b, 0x00,
    // (+648) \213 condensed string block "Brightness" (first quarter)
    8, 0xe2, 0xac, 0xca, 0xaa, 0xea,
    // (+654) \214 condensed string block "Brightness" (second quarter)
    8, 0x08, 0x6e, 0xaa, 0x6a, 0xca,
    // (+660) \215 condensed string block "Brightness" (third quarter)
    8, 0x80, 0xd8, 0x95, 0x95, 0xd4,
    // (+666) \216 condensed string block "Brightness" (fourth quarter)
    8, 0x00, 0xdb, 0x52, 0x89, 0xdb,
    // (+672) \217 Brightness bar image <auto> (first half)
    8, 0x00, 0xff, 0xd5, 0xaa, 0xff,
    // (+678) \220 Brightness bar image <auto> (last half)
    8, 0x00, 0xff, 0x55, 0xab, 0xff,
    // (+684) \221 Brightness bar image <1> (first half)
    8, 0x00, 0xff, 0xf0, 0xf8, 0xff,
    // (+690) \222 Brightness bar image <1> (last half)
    8, 0x00, 0xff, 0x01, 0x01, 0xff,
    // (+696) \223 Brightness bar image <2..4> (first half) or <4> (last half)
    8, 0x00, 0xff, 0xff, 0xff, 0xff,
    // (+702) \224 Brightness bar image <2> (last half)
    8, 0x00, 0xff, 0x01, 0x81, 0xff,
    // (+708) \225 Brightness bar image <3> (last half)
    8, 0x00, 0xff, 0xf1, 0xf9, 0xff
};

// Clear back frame buffer
void display_clear() {
    for (uint8_t i = 0; i < 16; i++) {
        fb_back[i] = 0ul;
    }
}

// Synchronize frame buffer planes
void display_sync() {
    for (uint8_t i = 0; i < 16; i++) {
        fb_front[i] = fb_back[i];
    }
}

// Display a character with specified font and coordinate
void display_putc(font_t f, uint8_t x, uint8_t y, uint8_t c) {
    uint8_t cm = c;
    uint8_t height = 0;
    uint16_t index = 0;
    uint8_t width = 0;
    uint32_t b;
    uint16_t bitmap_base = 0;
    volatile uint32_t *fbp = NULL;

    // Setup parameters
    switch (f) {
    case FONT_M0410:
        height = 10;
        index = pgm_read_word(font_index_m0410 + cm);
        width = 4;
        bitmap_base = (uint16_t) font_bitmap_m0410;
        break;
    case FONT_M0610:
        height = 10;
        index = pgm_read_word(font_index_m0610 + cm);
        width = 6;
        bitmap_base = (uint16_t) font_bitmap_m0610;
        break;
    case FONT_PP05:
        height = 5;
        index = pgm_read_word(font_index_pp05 + cm);
        width = pgm_read_byte(font_wbitmap_pp05 + index);
        bitmap_base = (uint16_t) font_wbitmap_pp05;
        break;
    }
    // Iterate for lines
    for (uint8_t i = 0; i < height; i++) {
        // Get destination of frame buffer to write
        fbp = &fb_back[y + i];
        // Get bitmap line
        if ((f & FONT_MASK) == FONT_PROPORTIONAL) {
            // For proportional fonts, add 1 width byte to the bitmap address
            b = pgm_read_byte(bitmap_base + index + 1 + i);
        } else {
            b = pgm_read_byte(bitmap_base + index + i);
        }
        // Overlay to back frame buffer
        if (x < width - 1) {
            *fbp |= b >> ((width - 1) - x);
        } else {
            *fbp |= b << (x - (width - 1));
        }
    }
}

// Display a vertically-scrolling character with specified font and coordinate
void display_putc_scroll(font_t f, uint8_t x, uint8_t y, uint8_t c_ex,
    uint8_t c_new, uint8_t frame) {
    uint8_t cm_ex = c_ex;
    uint8_t cm_new = c_new;
    uint8_t height = 0;
    uint16_t index_ex = 0, index_new = 0;
    uint8_t width_ex = 0, width_new = 0;
    uint32_t b;
    uint16_t bitmap_base = 0;
    volatile uint32_t *fbp = NULL;

    // Setup parameters
    switch (f) {
    case FONT_M0410:
        height = 10;
        index_ex = pgm_read_word(font_index_m0410 + cm_ex);
        index_new = pgm_read_word(font_index_m0410 + cm_new);
        width_ex = 4;
        width_new = 4;
        bitmap_base = (uint16_t) font_bitmap_m0410;
        break;
    case FONT_M0610:
        height = 10;
        index_ex = pgm_read_word(font_index_m0610 + cm_ex);
        index_new = pgm_read_word(font_index_m0610 + cm_new);
        width_ex = 6;
        width_new = 6;
        bitmap_base = (uint16_t) font_bitmap_m0610;
        break;
    case FONT_PP05:
        height = 5;
        index_ex = pgm_read_word(font_index_pp05 + cm_ex);
        index_new = pgm_read_word(font_index_pp05 + cm_new);
        width_ex = pgm_read_byte(font_wbitmap_pp05 + index_ex);
        width_new = pgm_read_byte(font_wbitmap_pp05 + index_new);
        bitmap_base = (uint16_t) font_wbitmap_pp05;
        break;
    }
    // Do nothing in case of invalid frame count
    if (frame > height) {
        return;
    }
    // Iterate for lines in new character
    for (uint8_t i = 0; i < frame - 1; i++) {
        // Get destination of frame buffer to write
        fbp = &fb_back[y + i];
        // Get bitmap line
        if (f == FONT_PP05) {
            // For proportional fonts, add 1 width byte to the bitmap address
            b = pgm_read_byte(bitmap_base + index_new + 1
                + (height + 1 - frame + i));
        } else {
            b = pgm_read_byte(bitmap_base + index_new
                + (height + 1 - frame + i));
        }
        // Overlay to back frame buffer
        if (x < width_new - 1) {
            *fbp |= b >> ((width_new - 1) - x);
        } else {
            *fbp |= b << (x - (width_new - 1));
        }
    }
    // Iterate for lines in existing character
    for (uint8_t i = 0; i < height - frame; i++) {
        // Get destination of frame buffer to write
        fbp = &fb_back[y + frame + i];
        // Get bitmap line
        if ((f & FONT_MASK) == FONT_PROPORTIONAL) {
            // For proportional fonts, add 1 width byte to the bitmap address
            b = pgm_read_byte(bitmap_base + index_ex + 1 + i);
        } else {
            b = pgm_read_byte(bitmap_base + index_ex + i);
        }
        // Overlay to back frame buffer
        if (x < width_ex - 1) {
            *fbp |= b >> ((width_ex - 1) - x);
        } else {
            *fbp |= b << (x - (width_ex - 1));
        }
    }
}
