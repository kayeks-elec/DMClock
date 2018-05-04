/*
 * event.h
 *
 *  Author: kayekss
 *  Target: unspecified
 */

#ifndef EVENT_H_
#define EVENT_H_

// Event entry
typedef struct {
    // On-event time
    struct {
        // Hours (0..23)
        uint8_t h;
        // Minutes (0..59)
        uint8_t m;
    } on;
    // Off-event time; {h=24, m=0} event lasts
    struct {
        // Hours (0..23)
        uint8_t h;
        // Minutes (0..59)
        uint8_t m;
    } off;
    // Event mask by day-of-week
    //   mask<7>  Saturdays (0: off; 1: on)
    //       <6>  Fridays (0: off; 1: on)
    //       <5>  Thursdays (0: off; 1: on)
    //       <4>  Wednesdays (0: off; 1: on)
    //       <3>  Tuesdays (0: off; 1: on)
    //       <2>  Mondays (0: off; 1: on)
    //       <1>  Sundays (0: off; 1: on)
    //       <0>  reserved
    uint8_t mask;
} event_t;

void event_clear(event_t* ev);
void event_fix_problems(event_t* ev);
bool event_output_state(event_t* ev, uint8_t h, uint8_t m, dow_t dow);

#endif
