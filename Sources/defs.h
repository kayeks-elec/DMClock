/*
 * DotMatrixClock2018/defs.h
 *
 *  Author: kayekss
 *  Target: ATmega328P, 20.000 MHz crystal oscillator
 */

#ifndef DEFS_DOTMATRIXCLOCK2018_H_
#define DEFS_DOTMATRIXCLOCK2018_H_

// USART receiver buffer length
#define RX_BUFFER_LENGTH                   32
// USART transmitter buffer length
#define TX_BUFFER_LENGTH                   32

// GPS NMEA message buffer length
#define MESSAGE_BUFFER_LENGTH             192

// EEPROM address map
#define EEREDUN_CONFIG_REDUNDANCY           7
#define EEREDUN_CONFIG_STRIDE             128
#define EEREDUN_CONFIG_BASE_TIMESTAMP  0x0010
#define EEREDUN_CONFIG_BASE_ENTITY     0x0080

// Number of event entries per item
#define NUM_EVENT_ENTRIES_PER_ITEM          8

// Interval for T5 tasks in milliseconds
#define T5_READ_KEYS_INTERVAL_MS           20
#define T5_READ_TEMPERATURE_INTERVAL_MS   600
#define T5_DRAW_SCREEN_INTERVAL_MS         20
#define T5_GET_LIGHT_LEVEL_INTERVAL_MS    200

// Retry count for reading RTC on startup
#define RETRY_COUNT_READ_RTC_ON_STARTUP    10

// Receiver timeout to GPS connection loss
#define GPS_CONNECTION_LOST_TIMEOUT_MS   5000

// T5 task
typedef struct {
    // Trigger interval in ticks 
    uint32_t interval;
    // Timestamp of last execution in ticks
    uint32_t timestamp;
} task5_t;

// T6 task
typedef struct {
    // Pending flag
    bool pending;
} task6_t;

// Enumeration table for states
typedef enum {
    ST_NORMAL_TIME_HM                = 0x01,
    ST_NORMAL_TIME_HMS               = 0x02,
    ST_NORMAL_DATE_WEEKOFDAY         = 0x04,
    ST_NORMAL_DATE_YEARS             = 0x08,
    ST_NORMAL_TEMPERATURE            = 0x0c,
    ST_NORMAL_GPS_STATUS             = 0x10,
    ST_CONFIG_USE_GPS                = 0x20,
    ST_CONFIG_SET_TIME_TOP           = 0x21,
    ST_CONFIG_SET_TIME_MOD_YH        = 0x40,
    ST_CONFIG_SET_TIME_MOD_YL        = 0x41,
    ST_CONFIG_SET_TIME_MOD_MO        = 0x42,
    ST_CONFIG_SET_TIME_MOD_D         = 0x43,
    ST_CONFIG_SET_TIME_MOD_H         = 0x44,
    ST_CONFIG_SET_TIME_MOD_MH        = 0x45,
    ST_CONFIG_SET_TIME_MOD_ML        = 0x46,
    ST_CONFIG_SET_TIME_MOD_CONFIRM   = 0x47,
    ST_CONFIG_RELAY_EVENT_TOP        = 0x22,
    ST_CONFIG_RELAY_EVENT_MOD        = 0x60,
    ST_CONFIG_RELAY_EVENT_MOD_ON_H   = 0x61,
    ST_CONFIG_RELAY_EVENT_MOD_ON_MH  = 0x62,
    ST_CONFIG_RELAY_EVENT_MOD_ON_ML  = 0x63,
    ST_CONFIG_RELAY_EVENT_MOD_OFF_H  = 0x64,
    ST_CONFIG_RELAY_EVENT_MOD_OFF_MH = 0x65,
    ST_CONFIG_RELAY_EVENT_MOD_OFF_ML = 0x66,
    ST_CONFIG_RELAY_EVENT_MASK       = 0x23,
    ST_CONFIG_BRIGHTNESS             = 0x24,
    ST_CONFIG_SAVE_CONFIRM           = 0x25,
    ST_MISC_LIGHT_SENSOR             = 0xe0
} state_t;
// Branching bit masks
enum {
    ST_NORMAL_BITS                 = 0x00,
    ST_CONFIG_SET_TIME_MOD_BITS    = 0x40,
    ST_CONFIG_RELAY_EVENT_MOD_BITS = 0x60
};
enum {
    ST_MASK              = 0xe0,
    ST_NORMAL_LOWER_MASK = 0xe3,
    ST_NORMAL_UPPER_MASK = 0xfc
};

// Enumeration table of GPS states
typedef enum {
    GP_ABSENT   = 0xff,
    GP_NO_FIX   = 0x00,
    GP_GPS_FIX  = 0x01,
    GP_DGPS_FIX = 0x02
} gpstate_t;

// Configuration structure
typedef struct {
    // State on startup
    state_t state_startup;
    // Use GPS input for time adjustment
    bool use_gps;
    // Relay events
    struct {
        // Events registered
        uint8_t count;
        // Event entries
        event_t ev[NUM_EVENT_ENTRIES_PER_ITEM];
    } relay[3];
    // Display brightness (1..4: fixed, 5..: automatic)
    uint8_t brightness;
} config_t;

#endif
