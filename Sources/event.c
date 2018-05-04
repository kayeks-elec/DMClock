/*
 * event.c
 *
 *  Author: kayekss
 *  Target: unspecified
 */ 

#include <stdbool.h>
#include <stdint.h>
#include "ctime.h"
#include "event.h"

// Clear event entry
inline void event_clear(event_t* ev) {
    ev->on.h = 0;
    ev->on.m = 0;
    ev->off.h = 0;
    ev->off.m = 0;
    ev->mask = ~0;
}

// Fix problems in event entry
inline void event_fix_problems(event_t* ev) {
    uint16_t on_mins, off_mins;

    // Void the entire entry if any value is out of range
    if (ev->on.h > 23 || ev->on.m > 59 ||
        ev->off.h > 24 || ev->off.m > 59) {
        event_clear(ev);
    }
    // Set off-event time to 24:00
    // ... if it is 24:>00 or time order is reversed
    on_mins = ev->on.h * 60 + ev->on.m;
    off_mins = ev->off.h * 60 + ev->off.m;
    if (off_mins > 24 * 60 || on_mins > off_mins) {
        ev->off.h = 24;
        ev->off.m = 0;
    }
    // Set the reserved bit
    ev->mask |= 0x01;
}

// Get the output state from specified clock time and day-of-week
inline bool event_output_state(event_t* ev, uint8_t h, uint8_t m, dow_t dow) {
    uint16_t on_mins, off_mins, mins;

    on_mins = ev->on.h * 60 + ev->on.m;
    off_mins = ev->off.h * 60 + ev->off.m;
    mins = h * 60 + m;
    return (ev->mask & (1 << dow)) && (mins >= on_mins) && (mins < off_mins);
}
