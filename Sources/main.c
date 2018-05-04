/*
 * DotMatrixClock2018/main.c
 *
 *  Author: kayekss
 *  Target: ATmega328P, 20.000 MHz crystal oscillator
 */

// System clock frequency in Hz
#define F_CPU  20000000ul

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "ctime.h"
#include "event.h"
#include "eeprom.h"
#include "eeprom_redundancy.h"
#include "keys.h"
#include "display.h"
#include "drawings.h"
#include "usart.h"
#include "nmea.h"
#include "twi.h"
#include "temp_adt7410.h"
#include "rtc_ds1307.h"
#include "adc.h"
#include "light_sensor.h"
#include "defs.h"

// -------- Global variables --------

// Elapsed time from startup in milliseconds
volatile uint32_t ticks = 0;

// Front frame buffer
extern uint32_t fb_front[16];

// ADC value of light sensor
uint16_t light_adc;

// USART receiver buffer
ringbuf_t rx;

// USART transmitter buffer
ringbuf_t tx;

// Working environment
struct {
    // Current status
    state_t status;
    // Current display brightness (0..4)
    uint8_t brightness;
    // Current internal time structure
    ctime_t ct;
    // Duplicated time structure used during modification
    ctime_t ct_mod;
    // Current week-of-day
    dow_t dow;
    // Week-of-day used during modification
    dow_t dow_mod;
    // GPS connection/tracking status
    struct {
        // Fix status
        gpstate_t status;
        // Satellites in use
        uint8_t sats_in_use;
    } gps;
    // T5 tasks
    struct {
        task5_t read_keys;
        task5_t read_temperature;
        task5_t draw_screen;
        task5_t get_light_level;
    } task5;
    // T6 tasks
    struct {
        task6_t save_ctime_to_rtc;
        task6_t check_relay_output;
        task6_t serial_output;
    } task6;
    // Key watchers
    key_t key0, key1;
    // Configuration structure and its duplication
    config_t config, config_mod;
    // Indexes used during relay setup in configuration mode
    struct {
        // Relay number currently indexing
        uint8_t r;
        // Event number currently indexing
        uint8_t e;
        // Day-of-week currently indexing
        dow_t dow;
    } relay_index;
    // Whether to save the configuration to EEPROM when it is done
    bool save_to_ee;
    // Buffer for EEPROM byte array
    uint8_t ee_blob[EEREDUN_CONFIG_STRIDE];
    // Temperature sensor status
    struct {
        // Result (0: no error, 1: error)
        bool result;
        // Temperature value
        temp_adt7410_t value;
        // Trigger flags
        uint8_t flags;
    } temperature;
    // Message buffer
    linebuf_t msg;
    // Ticks of last reception of '$' from USART
    uint32_t ticks_rx;
} env;

// Default clock time recalled on failure
ctime_t const ct_default = {
    1, 0, 1, 1, 0, 0, 0, 0
};

// EEPROM address map for configuration
eeredun_t const eer_config = {
    EEREDUN_CONFIG_REDUNDANCY,
    EEREDUN_CONFIG_STRIDE,
    { EEREDUN_CONFIG_BASE_TIMESTAMP, EEREDUN_CONFIG_BASE_ENTITY }
};

// -------- General functions --------

// Make an imprecise wait by busy-looping
void wait(uint32_t delay_ms) {
    uint32_t ticks_end_loop = ticks + delay_ms;

    while (ticks < ticks_end_loop);
}

// Get constrained value in range between specified
// ... minimum and maximum
inline uint8_t constrain(uint8_t n, uint8_t min, uint8_t max) {
    return n < min ? n : n > max ? max : n;
}

// Initialize T5 task
inline void t5_initialize(task5_t* t, uint32_t interval) {
    t->interval = interval;
    t->timestamp = 0;
}

// Check if the T5 task is triggered
inline bool t5_check_triggered(task5_t* t) {
    if (t->interval == 0) {
        return false;
    } else {
        return ticks >= t->timestamp + t->interval;
    }
}

// Set last-executed timestamp of the T5 task
inline void t5_set_timestamp(task5_t* t) {
    t->timestamp = ticks;
}

// Initialize T6 task
inline void t6_initialize(task6_t* t) {
    t->pending = false;
}

// Trigger T6 task
inline void t6_trigger(task6_t* t) {
    t->pending = true;
}

// Check if the T6 task is triggered
inline bool t6_check_triggered(task6_t* t) {
    return t->pending;
}

// Untrigger T6 task
inline void t6_done(task6_t* t) {
    t->pending = false;
}

// -------- Project-specific functions --------

// Setup EEPROM
void setup_eeprom() {
    EECR = (0 << EERIE) | (0 << EEMPE) | (0 << EEPE) | (0 << EERE);
    //     0b--XX0XXX  (-: reserved bits)
    //         |||||+-- EERE      EEPROM Read Enable
    //         ||||+--- EEPE      EEPROM Write Enable
    //         |||+---- EEMPE     EEPROM Master Write Enable 
    //         ||+----- EERIE     EEPROM Ready Interrupt Enable: no
    //         ++------ EEPM<1:0> EEPROM Programming Mode
}

// Setup I/O ports
void setup_io() {
    // == PORTB ==
    // PB7/XTAL2: crystal oscillator
    // PB6/XTAL1: crystal oscillator
    // PB5/SCK:   not in use, only for programming
    // PB4/MISO:  not in use, only for programming
    // PB3/MOSI:  not in use, only for programming
    // PB2:       Relay 2, normal-low output
    // PB1:       Relay 1, normal-low output
    // PB0:       Relay 0, normal-low output
    // == PORTC ==
    // PC6/RESET#: reset
    // PC5/SCL:    TWI serial bus clock
    // PC4/SDA:    TWI serial bus data
    // PC3:        not in use
    // PC2/ADC2:   Light sensor (photoresistor), analog input
    // PC1:        Push key 1, pulled-up input
    // PC0:        Push key 0, pulled-up input
    // == PORTD ==
    // PD7:     LED matrix module OE, output
    // PD6:     LED matrix module LAT, output
    // PD5:     LED matrix module CLK, output
    // PD4:     LED matrix module SIN3, output
    // PD3:     LED matrix module SIN2, output
    // PD2:     LED matrix module SIN1, output
    // PD1/TXD: USART transmitter, output
    // PD0/RXD: USART receiver, input
    PORTB = (0 << PORTB7) | (0 << PORTB6) | (0 << PORTB5) | (0 << PORTB4) |
        (0 << PORTB3) | (0 << PORTB2) | (0 << PORTB1) | (0 << PORTB0);
    //     0b00000000
    //       ++++++++-- PORTB<7:0> Port B Data
    DDRB = (0 << DDB7) | (0 << DDB6) | (0 << DDB5) | (0 << DDB4) |
        (0 << DDB3) | (1 << DDB2) | (1 << DDB1) | (1 << DDB0);
    //     0b00000111
    //       ++++++++-- DDRB<7:0> Port B Data Direction
    PORTC = (0 << PORTC6) | (0 << PORTC5) | (0 << PORTC4) |
        (0 << PORTC3) | (0 << PORTC2) | (1 << PORTC1) | (1 << PORTC0);
    //     0b-0000011  (-: reserved bits)
    //        +++++++-- PORTC<6:0> Port C Data
    DDRC = (0 << DDC6) | (0 << DDC5) | (0 << DDC4) |
        (0 << DDC3) | (0 << DDC2) | (0 << DDC1) | (0 << DDC0);
    //     0b-0000000  (-: reserved bits)
    //        +++++++-- DDRC<6:0> Port C Data Direction
    PORTD = (0 << PORTD7) | (0 << PORTD6) | (0 << PORTD5) | (0 << PORTD4) |
        (0 << PORTD3) | (0 << PORTD2) | (0 << PORTD1) | (0 << PORTD0);
    //     0b00000000
    //       ++++++++-- PORTD<7:0> Port D Data
    DDRD = (1 << DDD7) | (1 << DDD6) | (1 << DDD5) | (1 << DDD4) |
        (1 << DDD3) | (1 << DDD2) | (1 << DDD1) | (0 << DDD0);
    //     0b11111110
    //       ++++++++-- DDRD<7:0> Port D Data Direction
}

// Setup Timer/Counter 0
void setup_timer0() {
    TCCR0A = (0 << COM0A1) | (0 << COM0A0) | (0 << COM0B1) | (0 << COM0B0) |
        (1 << WGM01) | (0 << WGM00);
    //     0b0000--10  (-: reserved bits)
    //       ||||  ++-- WGM0<1:0>  Waveform Generation: CTC, TOP=OCR0A
    //       ||++------ COM0B<1:0> Compare Output Mode B: Normal
    //       ++-------- COM0A<1:0> Compare Output Mode A: Normal
    TCCR0B = (0 << FOC0A) | (0 << FOC0B) | (0 << WGM02) |
        (0 << CS02) | (1 << CS01) | (1 << CS00);
    //     0b00--0011  (-: reserved bits)
    //       ||  |+++-- CS0<2:0> Clock Select: clkI/O / 64
    //       ||  +----- WGM02    Waveform Generation  *see TCCR0A
    //       |+-------- FOC0B    Force Output Compare B  *unused
    //       +--------- FOC0A    Force Output Compare A  *unused
        TIMSK0 = (0 << OCIE0B) | (1 << OCIE0A) | (0 << TOIE0);
    //     0b-----010  (-: reserved bits)
    //            ||+-- TOIE0   Overflow Interrupt Enable: no
    //            |+--- OCIE0A  Compare A Match Int. Enable: yes
    //            +---- OCIE0B  Compare B Match Int. Enable: no

    // 20e+6[Hz(CPU)] / 4000[Hz(int)] / 64[prescaler]
    // ... actual value is 78.125, error rate +0.16%
    OCR0A = F_CPU / 4000 / 64 - 1;
    OCR0B = 0;

    // Clear counter
    TCNT0 = 0;
}

// Setup Timer/Counter 1
void setup_timer1() {
    TCCR1A = (0 << COM1A1) | (0 << COM1A0) | (0 << COM1B1) | (0 << COM1B0) |
        (0 << WGM11) | (0 << WGM10);
    //     0b0000--00  (-: reserved bits)
    //       ||||  ++-- WGM1<1:0>  Waveform Generation: CTC, TOP=OCR1A
    //       ||++------ COM1B<1:0> Compare Output Mode B: Normal
    //       ++-------- COM1A<1:0> Compare Output Mode A: Normal
    TCCR1B = (0 << ICNC1) | (0 << ICES1) | (0 << WGM13) | (1 << WGM12) |
        (0 << CS12) | (0 << CS11) | (1 << CS10);
    //     0b00-01001  (-: reserved bits)
    //       || ||+++-- CS1<2:0>  Clock Select: clkI/O / 1
    //       || ++----- WGM1<3:2> Waveform Generation  *see TCCR1A
    //       |+-------- ICES1     Input Capture Edge Select  *unused
    //       +--------- ICNC1     Input Capture Noise Canceler  *unused
    TCCR1C = (0 << FOC1A) | (0 << FOC1B);
    //     0b00------  (-: reserved bits)
    //       |+-------- FOC1B  Force Output Compare B  *unused
    //       +--------- FOC1A  Force Output Compare A  *unused
    TIMSK1 = (0 << ICIE1) | (0 << OCIE1B) | (1 << OCIE1A) | (0 << TOIE1);
    //     0b--0--010  (-: reserved bits)
    //         |  ||+-- TOIE1   Overflow Interrupt Enable: No
    //         |  |+--- OCIE1A  Compare A Match Interrupt Enable: yes
    //         |  +---- OCIE1B  Compare B Match Interrupt Enable: no
    //         +------- ICIE1   Input Capture Interrupt Enable: no

    // 20e+6[Hz(CPU)] / 1000[Hz(int)] / 1[prescaler]
    // ... actual value is 20000, error rate 0.00%
    OCR1A = F_CPU / 1000 / 1 - 1;
    OCR1B = 0;

    // Clear counter
    TCNT1 = 0;
}

// Setup USART 0
void setup_usart0() {
    UCSR0A = (0 << TXC0) | (0 << U2X0) | (0 << MPCM0);
    //     0bR0RRRR00  (R: read-only bits)
    //       |||||||+-- MPCM0  Multi-processor Communication Mode  *unused
    //       ||||||+--- U2X0   Double the USART Transmission Speed: no
    //       |||||+---- UPE0   USART Parity Error
    //       ||||+----- DOR0   Data Overrun
    //       |||+------ FE0    Frame Error
    //       ||+------- UDRE0  USART Data Register Empty
    //       |+-------- TXC0   USART Transmit Complete
    //       +--------- RXC0   USART Receive Complete
    UCSR0B = (1 << RXCIE0) | (0 << TXCIE0) | (0 << UDRIE0) | (1 << RXEN0) |
        (1 << TXEN0) | (0 << UCSZ02) | (0 << TXB80);
    //     0b10X110R0  (R: read-only bits)
    //       |||||||+-- TXB80   Transmit Data Bit 8
    //       ||||||+--- RXB80   Receive Data Bit 8
    //       |||||+---- UCSZ02  Character Size  *see UCSR0C
    //       ||||+----- TXEN0   Transmitter Enable: yes
    //       |||+------ RXEN0   Receiver Enable: yes
    //       ||+------- UDRIE0  Data Register Empty Interrupt Enable
    //       |+-------- TXCIE0  TX Complete Interrupt Enable: no
    //       +--------- RXCIE0  RX Complete Interrupt Enable: yes
    UCSR0C = (0 << UMSEL01) | (0 << UMSEL00) | (0 << UPM01) | (0 << UPM00) |
        (0 << USBS0) | (1 << UCSZ01) | (1 << UCSZ00) | (0 << UCPOL0);
    //     0b00000110
    //       |||||||+-- UCPOL0      Clock Polarity  *unused
    //       |||||++--- UCSZ0<1:0>  Character Size: 8-bit
    //       ||||+----- USBS0       USART Stop Bit Select: 1-bit
    //       ||++------ UPM0<1:0>   USART Parity Mode: Disabled
    //       ++-------- UMSEL0<1:0> USART Mode Select: Asynchronous USART

    // 20e+6[Hz(CPU)] / 9600[baud] / 16 * 1[doubler]
    // ... actual value is 130.21, error rate +0.16%
    UBRR0 = F_CPU / 9600 / 16 * 1 - 1;
}

// Setup Two-wire Serial Interface
void setup_twi() {
    TWCR = (0 << TWINT) | (0 << TWEA) | (0 << TWSTA) | (0 << TWSTO) |
        (1 << TWEN) | (0 << TWIE);
    //     0b00XXR1-0  (-: reserved bits, R: read-only bits)
    //       |||||| +-- TWIE   TWI Interrupt Enable: no
    //       |||||+---- TWEN   TWI Enable: yes
    //       ||||+----- TWWC   TWI Write Collision Flag
    //       |||+------ TWSTO  TWI Stop Condition
    //       ||+------- TWSTA  TWI Start Condition
    //       |+-------- TWEA   TWI Enable Acknowledge: no
    //       +--------- TWINT  TWI Interrupt Flag
    TWSR = (0 << TWPS1) | (0 << TWPS0);
    //     0bRRRRR-00  (-: reserved bits, R: read-only bits)
    //       ||||| ++-- TWPS<1:0> TWI Prescaler: 1
    //       +++++----- TWS<7:3>  TWI Status Bit
    TWDR = 0x00;

    // (20e+6[Hz(CPU)] / 100e+3[Hz(SCL)] - 16) / 1[prescaler] / 2
    // ... actual value is 92, error rate 0.00%
    TWBR = (F_CPU / 100000 - 16) / 1 / 2;
}

// Setup Analog to Digital Converter
void setup_adc() {
    ADMUX = (0 << REFS1) | (0 << REFS0) | (0 << ADLAR);
    //     0b000-XXXX  (-: reserved bits)
    //       ||| ++++-- MUX<3:0>  Analog Channel Selection
    //       ||+------- ADLAR     ADC Left Adjust: no
    //       ++-------- REFS<1:0> Reference Selection: AREF
    ADCSRA = (1 << ADEN) | (0 << ADATE) | (0 << ADIE) | (0 << ADIF) |
        (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
    //     0b1X0X0111
    //       |||||+++-- ADPS<2:0> ADC Prescaler Select: 128
    //       ||||+----- ADIE      ADC Interrupt Enable: no
    //       |||+------ ADIF      ADC Interrupt Flag
    //       ||+------- ADATE     ADC Auto Trigger Enable: no
    //       |+-------- ADSC      ADC Start Conversion
    //       +--------- ADEN      ADC Enable: yes
    ADCSRB = (ADCSRB & 0x40) | (0 << ADTS2) | (0 << ADTS1) | (0 << ADTS0);
    //     0b-?---000  (-: reserved bits, ?: bits used in other functions)
    //        |   +++-- ADTS<2:0> ADC Auto Trigger Source  *unused
    //        +-------- ACME      Analog Comparator Multiplexer Enable
    DIDR0  = (0 << ADC5D) | (0 << ADC4D) | (0 << ADC3D) | (1 << ADC2D) |
        (0 << ADC1D) | (0 << ADC0D);
    //     0b--000100  (-: reserved bits)
    //         ++++++-- ADC<5:0>D  ADC Digital Input Disable
}

// Timer/Counter 0 Compare Match A interrupt vector
ISR(TIMER0_COMPA_vect) {
    // Line to display for this time (0..15)
    static uint8_t y = 0;
    // PWM phase (0..3)
    static uint8_t pwm = 0;
    // Frame buffer line to send to the display for this time
    uint32_t fbline;
    
    // Get target line from frame buffer
    fbline = pwm < env.brightness ? fb_front[y] : 0x0000000ul;

    // Set latch to low
    PORTD &= ~(1 << PORTD6);
    // Send buffer line as 16-bit serial data
    for (uint8_t i = 0; i < 16; i++) {
        // Set clock to low
        PORTD &= ~(1 << PORTD5);
        // Set data;
        //   PD2: line driver
        //   PD3: column driver for left half
        //   PD4: column driver for right half
        PORTD = (PORTD & 0xe3) |
            (i == y ? (1 << PORTD2) : 0x00) |
            ((fbline & (0x00010000ul << i)) ? (1 << PORTD3) : 0x00) |
            ((fbline & (0x00000001ul << i)) ? (1 << PORTD4) : 0x00);
        // Set clock to high; data is read by the display
        PORTD |= (1 << PORTD5);
    }
    // Set latch to low; display is updated
    PORTD |= (1 << PORTD6);

    // Advance line
    if (y == 15) {
        y = 0;
        // Advance PWM phase
        pwm = (pwm + 1) & 0x03;
    } else {
        y++;
    }
}

// Timer/Counter 0 Compare Match A interrupt vector
ISR(TIMER1_COMPA_vect) {
    uint8_t carry;
    
    // Increment ticks
    ticks++;
    // Increment clock ticks
    carry = ctime_increment_tick(&env.ct);
    if (carry & (1 << 0)) {
        // Validate clock time on every second
        ctime_check_error(&env.ct);
    }
    if (carry & (1 << 1)) {
        // Recalculate week-of-day on every minute
        env.dow = dayofweek(&env.ct);
        // Trigger tasks as clock time advances
        t6_trigger(&env.task6.save_ctime_to_rtc);
        t6_trigger(&env.task6.check_relay_output);
        t6_trigger(&env.task6.serial_output);
    }
    // Check if GPS connection is timed out
    if (ticks >= env.ticks_rx + GPS_CONNECTION_LOST_TIMEOUT_MS) {
        env.gps.status = GP_ABSENT;
        env.gps.sats_in_use = 0;
    }
}

// USART Receive Complete interrupt vector
ISR(USART_RX_vect) {
    uint8_t c = UDR0;
    uint32_t ticks0 = ticks;
    
    ringbuf_put(&rx, c);
    if (c == '$') {
        env.ticks_rx = ticks0;
    }
}

// USART Data Register Empty interrupt vector
ISR(USART_UDRE_vect) {
    uint8_t c = 0;

    if (ringbuf_available(&tx)) {
        ringbuf_get(&tx, &c);
        usart_putc(c);
    } else {
        // Disable further interrupts
        UCSR0B &= ~(1 << UDRIE0);
    }
}

// Setup configuration structure to fallback value 
void setup_fallback_config(config_t* config) {
    config->state_startup = ST_NORMAL_TIME_HM | ST_NORMAL_DATE_WEEKOFDAY;
    config->use_gps = false;
    for (uint8_t j = 0; j < 3; j++) {
        config->relay[j].count = 0;
        for (uint8_t i = 0; i < NUM_EVENT_ENTRIES_PER_ITEM; i++) {
            event_clear(&config->relay[j].ev[i]);
        }
    }
    config->brightness = 2;
}

// Import configuration structure from EEPROM byte array
void import_config_from_blob(config_t* config, uint8_t* blob) {
    uint8_t const stride_j = 5 * NUM_EVENT_ENTRIES_PER_ITEM + 1;
    uint8_t const stride_i = 5;
    event_t ev_temp = { { 0, 0 }, { 0, 0 }, ~0 };

    // Load default value when invalid startup state is read
    if (blob[0] & ST_MASK) {
        config->state_startup = ST_NORMAL_TIME_HMS | ST_NORMAL_DATE_WEEKOFDAY;
    } else {
        config->state_startup = blob[0] & ~ST_MASK;
    }
    config->use_gps = !!blob[1];
    for (uint8_t j = 0; j < 3; j++) {
        // Void all events when event count is out-of-range
        if (blob[2 + stride_j * j] > NUM_EVENT_ENTRIES_PER_ITEM) {
            config->relay[j].count = 0;
        } else {
            config->relay[j].count = blob[2 + stride_j * j];
        }
        for (uint8_t i = 0; i < config->relay[j].count; i++) {
            ev_temp.on.h = blob[2 + stride_j * j + stride_i * i + 1];
            ev_temp.on.m = blob[2 + stride_j * j + stride_i * i + 2];
            ev_temp.off.h = blob[2 + stride_j * j + stride_i * i + 3];
            ev_temp.off.m = blob[2 + stride_j * j + stride_i * i + 4];
            ev_temp.mask = blob[2 + stride_j * j + stride_i * i + 5] | 0x01;
            event_fix_problems(&ev_temp);
            config->relay[j].ev[i] = ev_temp;
        }
        event_clear(&ev_temp);
        for (uint8_t i = config->relay[j].count;
            i < NUM_EVENT_ENTRIES_PER_ITEM; i++) {
            config->relay[j].ev[i] = ev_temp;
        }
    }
    config->brightness = constrain(blob[2 + stride_j * 3], 1, 5);
}

// Export configuration structure to EEPROM byte array
void export_config_to_blob(config_t* config, uint8_t* blob) {
    uint8_t const stride_j = 5 * NUM_EVENT_ENTRIES_PER_ITEM + 1;
    uint8_t const stride_i = 5;

    blob[0] = config->state_startup;
    blob[1] = config->use_gps ? 0x01 : 0x00;
    for (uint8_t j = 0; j < 3; j++) {
        blob[2 + stride_j * j] = config->relay[j].count;
        for (uint8_t i = 0; i < NUM_EVENT_ENTRIES_PER_ITEM; i++) {
            blob[2 + stride_j * j + stride_i * i + 1]
                = config->relay[j].ev[i].on.h;
            blob[2 + stride_j * j + stride_i * i + 2]
                = config->relay[j].ev[i].on.m;
            blob[2 + stride_j * j + stride_i * i + 3]
                = config->relay[j].ev[i].off.h;
            blob[2 + stride_j * j + stride_i * i + 4]
                = config->relay[j].ev[i].off.m;
            blob[2 + stride_j * j + stride_i * i + 5]
                = config->relay[j].ev[i].mask | 0x01;
        }
    }
    blob[2 + stride_j * 3] = config->brightness;
}

// Set display brightness from the light level and configuration
void set_brightness(uint8_t level) {
    if (env.config.brightness <= 4) {
        env.brightness = env.config.brightness;
    } else {
        env.brightness = level;
    }
}

// T5: read keys and trigger events
void task5_read_keys() {
    uint8_t* u8p = NULL;

    if (t5_check_triggered(&env.task5.read_keys)) {
        t5_set_timestamp(&env.task5.read_keys);

        // Poll keys
        uint8_t pinc = PINC;
        key_poll(&env.key0, pinc & (1 << PINC0));
        key_poll(&env.key1, pinc & (1 << PINC1));
        // Trigger events
        if ((env.status & ST_MASK) == ST_NORMAL_BITS) {
            if (key_is_pressed(&env.key0)) {
                switch (env.status) {
                case ST_NORMAL_TIME_HM | ST_NORMAL_DATE_WEEKOFDAY:
                default:
                    env.status = ST_NORMAL_TIME_HM | ST_NORMAL_DATE_YEARS;
                    break;
                case ST_NORMAL_TIME_HM | ST_NORMAL_DATE_YEARS:
                    env.status = ST_NORMAL_TIME_HM | ST_NORMAL_TEMPERATURE;
                    break;
                case ST_NORMAL_TIME_HM | ST_NORMAL_TEMPERATURE:
                    env.status = ST_NORMAL_TIME_HM | ST_NORMAL_GPS_STATUS;
                    break;
                case ST_NORMAL_TIME_HM | ST_NORMAL_GPS_STATUS:
                    env.status = ST_NORMAL_TIME_HMS | ST_NORMAL_DATE_WEEKOFDAY;
                    break;
                case ST_NORMAL_TIME_HMS | ST_NORMAL_DATE_WEEKOFDAY:
                    env.status = ST_NORMAL_TIME_HMS | ST_NORMAL_DATE_YEARS;
                    break;
                case ST_NORMAL_TIME_HMS | ST_NORMAL_DATE_YEARS:
                    env.status = ST_NORMAL_TIME_HMS | ST_NORMAL_TEMPERATURE;
                    break;
                case ST_NORMAL_TIME_HMS | ST_NORMAL_TEMPERATURE:
                    env.status = ST_NORMAL_TIME_HMS | ST_NORMAL_GPS_STATUS;
                    break;
                case ST_NORMAL_TIME_HMS | ST_NORMAL_GPS_STATUS:
                    env.status = ST_NORMAL_TIME_HM | ST_NORMAL_DATE_WEEKOFDAY;
                    break;
                }
            }
            if (key_is_pressed(&env.key1)) {
                // Enter into configuration mode;
                // ... prepare configuration structure
                env.config_mod = env.config;
                env.config_mod.state_startup = env.status;
                env.status = ST_CONFIG_USE_GPS;
            }
        } else {
            switch (env.status) {
            case ST_CONFIG_USE_GPS:
                if (key_is_pressed(&env.key0)) {
                    // Toggle ballot box
                    env.config_mod.use_gps = !env.config_mod.use_gps;
                }
                if (key_is_pressed(&env.key1)) {
                    // Move to next configuration state
                    if (env.config_mod.use_gps) {
                        env.relay_index.r = 0;
                        env.status = ST_CONFIG_RELAY_EVENT_TOP;
                    } else {
                        env.status = ST_CONFIG_SET_TIME_TOP;
                    }
                }
                break;
            case ST_CONFIG_SET_TIME_TOP:
                if (key_is_pressed(&env.key0)) {
                    // Enter into time modification
                    env.ct_mod = env.ct;
                    env.ct_mod.s = 0;
                    env.ct_mod.ms = 0;
                    env.dow_mod = dayofweek(&env.ct_mod);
                    env.status = ST_CONFIG_SET_TIME_MOD_YH;
                }
                if (key_is_pressed(&env.key1)) {
                    // Move to next configuration state
                    env.relay_index.r = 0;
                    env.status = ST_CONFIG_RELAY_EVENT_TOP;
                }
                break;
            case ST_CONFIG_SET_TIME_MOD_YH:
                if (key_is_pressed(&env.key0)) {
                    // Change high digit of years
                    env.ct_mod.yh = env.ct_mod.yh >= 9 ? 0 : env.ct_mod.yh + 1;
                    // Update day-of-week
                    env.dow_mod = dayofweek(&env.ct_mod);
                }
                if (key_is_pressed(&env.key1)) {
                    // Move to next digit
                    env.status = ST_CONFIG_SET_TIME_MOD_YL;
                }
                break;
            case ST_CONFIG_SET_TIME_MOD_YL:
                if (key_is_pressed(&env.key0)) {
                    // Change low digit of years
                    env.ct_mod.yl = env.ct_mod.yl >= 9 ? 0 : env.ct_mod.yl + 1;
                    // Update day-of-week
                    env.dow_mod = dayofweek(&env.ct_mod);
                }
                if (key_is_pressed(&env.key1)) {
                    // Move to next digit
                    env.status = ST_CONFIG_SET_TIME_MOD_MO;
                }
                break;
            case ST_CONFIG_SET_TIME_MOD_MO:
                if (key_is_pressed(&env.key0)) {
                    // Change months
                    env.ct_mod.mo = env.ct_mod.mo >= 12 ? 1 : env.ct_mod.mo + 1;
                    // Update day-of-week
                    env.dow_mod = dayofweek(&env.ct_mod);
                }
                if (key_is_pressed(&env.key1)) {
                    // Correct days to fit
                    if (env.ct_mod.d > days_in_month(&env.ct_mod)) {
                        env.ct_mod.d = days_in_month(&env.ct_mod);
                        // Update day-of-week
                        env.dow_mod = dayofweek(&env.ct_mod);
                    }
                    // Move to next digit
                    env.status = ST_CONFIG_SET_TIME_MOD_D;
                }
                break;
            case ST_CONFIG_SET_TIME_MOD_D:
                if (key_is_pressed(&env.key0)) {
                    // Change days
                    env.ct_mod.d = env.ct_mod.d >= days_in_month(&env.ct_mod) ?
                        1 : env.ct_mod.d + 1;
                    // Update day-of-week
                    env.dow_mod = dayofweek(&env.ct_mod);
                }
                if (key_is_pressed(&env.key1)) {
                    // Move to next digit
                    env.status = ST_CONFIG_SET_TIME_MOD_H;
                }
                break;
            case ST_CONFIG_SET_TIME_MOD_H:
                if (key_is_pressed(&env.key0)) {
                    // Change hours
                    env.ct_mod.h = env.ct_mod.h >= 23 ? 0 : env.ct_mod.h + 1;
                }
                if (key_is_pressed(&env.key1)) {
                    // Move to next digit
                    env.status = ST_CONFIG_SET_TIME_MOD_MH;
                }
                break;
            case ST_CONFIG_SET_TIME_MOD_MH:
                if (key_is_pressed(&env.key0)) {
                    // Change high digit of minutes
                    env.ct_mod.m = env.ct_mod.m >= 50 ?
                        env.ct_mod.m - 50 : env.ct_mod.m + 10;
                }
                if (key_is_pressed(&env.key1)) {
                    // Move to next digit
                    env.status = ST_CONFIG_SET_TIME_MOD_ML;
                }
                break;
            case ST_CONFIG_SET_TIME_MOD_ML:
                if (key_is_pressed(&env.key0)) {
                    // Change low digit of minutes
                    env.ct_mod.m = env.ct_mod.m % 10 == 9 ?
                        env.ct_mod.m - 9 : env.ct_mod.m + 1;
                }
                if (key_is_pressed(&env.key1)) {
                    // Move to confirmation
                    env.status = ST_CONFIG_SET_TIME_MOD_CONFIRM;
                }
                break;
            case ST_CONFIG_SET_TIME_MOD_CONFIRM:
                if (key_is_pressed(&env.key0)) {
                    // Return discarding modifications
                    env.status = ST_CONFIG_SET_TIME_TOP;
                }
                if (key_is_pressed(&env.key1)) {
                    // Return saving modifications to a new clock time
                    env.ct = env.ct_mod;
                    env.dow = dayofweek(&env.ct);
                    env.status = ST_CONFIG_SET_TIME_TOP;
                    // Trigger tasks as clock time is modified
                    t6_trigger(&env.task6.save_ctime_to_rtc);
                    t6_trigger(&env.task6.check_relay_output);
                }
                break;
            case ST_CONFIG_RELAY_EVENT_TOP:
                if (key_is_pressed(&env.key0)) {
                    // Enter into relay event modification
                    for (uint8_t i = env.config_mod.relay[env.relay_index.r]
                        .count; i < NUM_EVENT_ENTRIES_PER_ITEM; i++) {
                        event_clear(
                            &env.config_mod.relay[env.relay_index.r].ev[i]);
                    }
                    env.config_mod.relay[env.relay_index.r].count = 0;
                    env.relay_index.e = 0;
                    env.status = ST_CONFIG_RELAY_EVENT_MOD;
                }
                if (key_is_pressed(&env.key1)) {
                    // Move to next relay or next configuration state
                    if (env.relay_index.r >= 2) {
                        env.status = ST_CONFIG_BRIGHTNESS;
                    } else {
                        env.relay_index.r++;
                    }
                }
                break;
            case ST_CONFIG_RELAY_EVENT_MOD:
                if (key_is_pressed(&env.key0)) {
                    // End relay event setup
                    env.status = ST_CONFIG_RELAY_EVENT_TOP;
                }
                if (key_is_pressed(&env.key1)) {
                    // Enter into relay event time modification
                    env.status = ST_CONFIG_RELAY_EVENT_MOD_ON_H;
                }
                break;
            case ST_CONFIG_RELAY_EVENT_MOD_ON_H:
                if (key_is_pressed(&env.key0)) {
                    // Change on-event hours
                    u8p = &env.config_mod.relay[env.relay_index.r]
                        .ev[env.relay_index.e].on.h;
                    *u8p = *u8p >= 23 ? 0 : *u8p + 1;
                }
                if (key_is_pressed(&env.key1)) {
                    // Move to next digit
                    env.status = ST_CONFIG_RELAY_EVENT_MOD_ON_MH;
                }
                break;
            case ST_CONFIG_RELAY_EVENT_MOD_ON_MH:
                if (key_is_pressed(&env.key0)) {
                    // Change high digit of on-event minutes
                    u8p = &env.config_mod.relay[env.relay_index.r]
                        .ev[env.relay_index.e].on.m;
                    *u8p = *u8p >= 50 ? *u8p - 50 : *u8p + 10;
                }
                if (key_is_pressed(&env.key1)) {
                    // Move to next digit
                    env.status = ST_CONFIG_RELAY_EVENT_MOD_ON_ML;
                }
                break;
            case ST_CONFIG_RELAY_EVENT_MOD_ON_ML:
                if (key_is_pressed(&env.key0)) {
                    // Change low digit of on-event minutes
                    u8p = &env.config_mod.relay[env.relay_index.r]
                        .ev[env.relay_index.e].on.m;
                    *u8p = *u8p % 10 == 9 ? *u8p - 9 : *u8p + 1;
                }
                if (key_is_pressed(&env.key1)) {
                    // Move to next digit
                    env.status = ST_CONFIG_RELAY_EVENT_MOD_OFF_H;
                }
                break;
            case ST_CONFIG_RELAY_EVENT_MOD_OFF_H:
                if (key_is_pressed(&env.key0)) {
                    // Change off-event hours
                    u8p = &env.config_mod.relay[env.relay_index.r]
                        .ev[env.relay_index.e].off.h;
                    *u8p = *u8p >= 24 ? 0 : *u8p + 1;
                }
                if (key_is_pressed(&env.key1)) {
                    // Move to next digit
                    if (env.config_mod.relay[env.relay_index.r]
                        .ev[env.relay_index.e].off.h == 24) {
                        // If hours is 24, set minutes to zero
                        // ... and skip the configuration state
                        env.config_mod.relay[env.relay_index.r]
                        .ev[env.relay_index.e].off.m = 0;
                        env.relay_index.dow = DOW_SUNDAY;
                        env.status = ST_CONFIG_RELAY_EVENT_MASK;
                    } else {
                        env.status = ST_CONFIG_RELAY_EVENT_MOD_OFF_MH;
                    }
                }
                break;
            case ST_CONFIG_RELAY_EVENT_MOD_OFF_MH:
                if (key_is_pressed(&env.key0)) {
                    // Change high digit of off-event minutes
                    u8p = &env.config_mod.relay[env.relay_index.r]
                        .ev[env.relay_index.e].off.m;
                    *u8p = *u8p >= 50 ? *u8p - 50 : *u8p + 10;
                }
                if (key_is_pressed(&env.key1)) {
                    // Move to next digit
                    env.status = ST_CONFIG_RELAY_EVENT_MOD_OFF_ML;
                }
                break;
            case ST_CONFIG_RELAY_EVENT_MOD_OFF_ML:
                if (key_is_pressed(&env.key0)) {
                    // Change low digit of off-event minutes
                    u8p = &env.config_mod.relay[env.relay_index.r]
                        .ev[env.relay_index.e].off.m;
                    *u8p = *u8p % 10 == 9 ? *u8p - 9 : *u8p + 1;
                }
                if (key_is_pressed(&env.key1)) {
                    // Move to event mask setting
                    env.relay_index.dow = DOW_SUNDAY;
                    env.status = ST_CONFIG_RELAY_EVENT_MASK;
                }
                break;
            case ST_CONFIG_RELAY_EVENT_MASK:
                if (key_is_pressed(&env.key0)) {
                    // Toggle ballot box of currently indexing day-of-week
                    env.config_mod.relay[env.relay_index.r]
                        .ev[env.relay_index.e].mask
                        ^= (1 << (uint8_t) env.relay_index.dow);
                }
                if (key_is_pressed(&env.key1)) {
                    if (env.relay_index.dow == DOW_SATURDAY) {
                        // Add registered event count
                        env.config_mod.relay[env.relay_index.r].count++;
                        if (env.config_mod.relay[env.relay_index.r].count
                            < NUM_EVENT_ENTRIES_PER_ITEM) {
                            // Move index to next event and continue
                            env.relay_index.e++;
                            env.status = ST_CONFIG_RELAY_EVENT_MOD;
                        } else {
                            // Return if all event slots are filled
                            env.status = ST_CONFIG_RELAY_EVENT_TOP;
                        }
                    } else {
                        // Move index to next day
                        env.relay_index.dow++;
                    }
                }
                break;
            case ST_CONFIG_BRIGHTNESS:
                if (key_is_pressed(&env.key0)) {
                    // Change brightness value
                    env.config_mod.brightness =
                        env.config_mod.brightness >= 5 ? 1 :
                        env.config_mod.brightness + 1;
                }
                if (key_is_pressed(&env.key1)) {
                    // Merge configuration
                    env.config = env.config_mod;
                    // Trigger task as relay events are modified
                    t6_trigger(&env.task6.check_relay_output);
                    // Return to save confirmation
                    env.save_to_ee = false;
                    env.status = ST_CONFIG_SAVE_CONFIRM;
                }
                break;
            case ST_CONFIG_SAVE_CONFIRM:
                if (key_is_pressed(&env.key0)) {
                    // Change brightness value
                    env.save_to_ee = !env.save_to_ee;
                }
                if (key_is_pressed(&env.key1)) {
                    if (env.save_to_ee) {
                        // Save configuration to EEPROM
                        export_config_to_blob(&env.config_mod, env.ee_blob);
                        eeprom_redun_write((eeredun_t*) &eer_config, env.ee_blob);
                    }
                    // Return to normal mode
                    env.status = env.config.state_startup;
                }
                break;
            default:
                break;
            }
        }
    }
}

// T5: Read temperature from sensor
void task5_read_temperature() {
    if (t5_check_triggered(&env.task5.read_temperature)) {
        t5_set_timestamp(&env.task5.read_temperature);

        // Read light sensor ADC
        env.temperature.result = temp_adt7410_read_temperature(
            &env.temperature.value, &env.temperature.flags,
            CONFIG_ADT7410_RESOL_13BITS);
    }
}

// T5: Draw screen
void task5_draw_screen() {
    uint16_t blinker = 0;
    uint16_t mask = ~0;
    uint8_t icon_l = ' ';
    uint8_t icon_r = ' ';

    if (t5_check_triggered(&env.task5.draw_screen)) {
        t5_set_timestamp(&env.task5.draw_screen);

        // Set blinker
        blinker = ((uint8_t) ticks >> 3) & 0x01 ? ~0 : 0;
        // Clear back frame buffer
        display_clear();
        switch (env.status & ST_MASK) {
        case ST_NORMAL_BITS:
            // Draw date/time/temperature/GPS in normal states
            // Lower region (y=6..15)
            switch (env.status & ST_NORMAL_LOWER_MASK) {
            case ST_NORMAL_TIME_HM:
                // Draw clock time; hours and minutes
                draw_time_hm(&env.ct, ~0);
                break;
            case ST_NORMAL_TIME_HMS:
                // Draw clock time; hours, minutes and seconds
                draw_time_hms(&env.ct, ~0);
                break;
            default:
                break;
            }
            // Upper region (y=0..4)
            switch (env.status & ST_NORMAL_UPPER_MASK) {
            case ST_NORMAL_DATE_WEEKOFDAY:
                // Draw clock date; months, days and week-of-day
                draw_date_dayofweek(&env.ct, env.dow, ~0);
                break;
            case ST_NORMAL_DATE_YEARS:
                // Draw clock date; months, days and years
                draw_date_year(&env.ct, ~0);
                break;
            case ST_NORMAL_TEMPERATURE:
                // Draw temperature status
                draw_temperature(
                    env.temperature.result, env.temperature.value.sign,
                    env.temperature.value.integer,
                    env.temperature.value.fraction_x10k, ~0);
                break;
            case ST_NORMAL_GPS_STATUS:
                // Draw GPS connection/tracking status
                draw_gps_status(env.gps.status, env.gps.sats_in_use, ~0);
                break;
            default:
                break;
            }
            break;
        case ST_CONFIG_SET_TIME_MOD_BITS:
            // Draw common configuration screen SET_TIME_MOD*
            switch (env.status) {
            case ST_CONFIG_SET_TIME_MOD_YH:
                mask = ~(1 << 12) | blinker;
                break;
            case ST_CONFIG_SET_TIME_MOD_YL:
                mask = ~(1 << 11) | blinker;
                break;
            case ST_CONFIG_SET_TIME_MOD_MO:
                mask = ~(1 << 10) | blinker;
                break;
            case ST_CONFIG_SET_TIME_MOD_D:
                mask = ~(1 << 9) | blinker;
                break;
            case ST_CONFIG_SET_TIME_MOD_H:
                mask = ~(1 << 7) | blinker;
                break;
            case ST_CONFIG_SET_TIME_MOD_MH:
                mask = ~(1 << 6) | blinker;
                break;
            case ST_CONFIG_SET_TIME_MOD_ML:
                mask = ~(1 << 5) | blinker;
                break;
            case ST_CONFIG_SET_TIME_MOD_CONFIRM:
                mask = ~((1 << 12) | (1 << 11) | (1 << 10) |
                    (1 << 9) | (1 << 8) | (1 << 7) | (1 << 6) |
                    (1 << 5)) | blinker;
                break;
            }
            icon_l = env.status == ST_CONFIG_SET_TIME_MOD_CONFIRM ?
                '\204' : '\203';
            draw_config_set_time_mod(
                &env.ct_mod, env.dow_mod, icon_l, '\205', mask
                );
            break;
        case ST_CONFIG_RELAY_EVENT_MOD_BITS:
            // Draw common configuration screen RELAY_EVENT_MOD*
            switch (env.status) {
            case ST_CONFIG_RELAY_EVENT_MOD:
                mask = ~(1 << 8) | blinker;
                break;
            case ST_CONFIG_RELAY_EVENT_MOD_ON_H:
                mask = ~(1 << 7) | blinker;
                break;
            case ST_CONFIG_RELAY_EVENT_MOD_ON_MH:
                mask = ~(1 << 6) | blinker;
                break;
            case ST_CONFIG_RELAY_EVENT_MOD_ON_ML:
                mask = ~(1 << 5) | blinker;
                break;
            case ST_CONFIG_RELAY_EVENT_MOD_OFF_H:
                mask = ~(1 << 4) | blinker;
                break;
            case ST_CONFIG_RELAY_EVENT_MOD_OFF_MH:
                mask = ~(1 << 3) | blinker;
                break;
            case ST_CONFIG_RELAY_EVENT_MOD_OFF_ML:
                mask = ~(1 << 2) | blinker;
                break;
            }
            icon_l = env.status == ST_CONFIG_RELAY_EVENT_MOD ?
                '\200' : '\203';
            icon_r = env.status == ST_CONFIG_RELAY_EVENT_MOD ?
                '\201' : '\205';
            draw_config_relay_event_mod(
                env.relay_index.e,
                &env.config_mod.relay[env.relay_index.r]
                .ev[env.relay_index.e], icon_l, icon_r, mask);
            break;
        default:
            switch (env.status) {
            case ST_CONFIG_USE_GPS:
                // Draw configuration screen USE_GPS
                draw_config_use_gps(
                    env.config_mod.use_gps, ~(1 << 5) | blinker);
                break;
            case ST_CONFIG_SET_TIME_TOP:
                // Draw configuration screen SET_TIME_TOP
                draw_config_set_time_top(~0);
                break;
            case ST_CONFIG_RELAY_EVENT_TOP:
                // Draw configuration screen RELAY_EVENT_TOP
                draw_config_relay_event_top(
                    env.relay_index.r,
                    env.config_mod.relay[env.relay_index.r].count,
                    ~0);
                break;
            case ST_CONFIG_RELAY_EVENT_MASK:
                // Draw configuration screen RELAY_EVENT_MASK
                draw_config_relay_event_mask(
                    env.relay_index.e,
                    &env.config_mod.relay[env.relay_index.r]
                    .ev[env.relay_index.e], env.relay_index.dow,
                    ~(1 << 5) | blinker);
                break;
            case ST_CONFIG_BRIGHTNESS:
                // Draw configuration screen BRIGHTNESS
                draw_config_brightness(
                    env.config_mod.brightness, ~(1 << 5) | blinker);
                break;
            case ST_CONFIG_SAVE_CONFIRM:
                // Draw configuration screen SAVE_CONFIRM
                draw_config_save_confirm(env.save_to_ee, ~(1 << 5) | blinker);
                break;
            case ST_MISC_LIGHT_SENSOR:
                // Draw the ADC value of light sensor
                draw_light_adc(light_adc);
                break;
            }
            break;
        }
        // Synchronize frame buffers
        display_sync();
    }
}

// T5: Read light sensor and set brightness
void task5_set_brightness() {
    if (t5_check_triggered(&env.task5.get_light_level)) {
        t5_set_timestamp(&env.task5.get_light_level);

        // Read light sensor ADC
        light_adc = read_adc(2);
        // Set brightness with the ADC value
        set_brightness(adc_to_brightness_level(light_adc));
    }
}

// T6: Save current clock time to RTC
void task6_save_ctime_to_rtc() {
    if (t6_check_triggered(&env.task6.save_ctime_to_rtc)) {
        t6_done(&env.task6.save_ctime_to_rtc);

        // Write current clock time to RTC
        rtc_ds1307_write_clock(&env.ct, env.dow);
    }
}

// T6: Check event state and apply to relay output
void task6_check_relay_output() {
    uint8_t port = 0x00;
    uint8_t h, m;
    uint8_t dow;

    if (t6_check_triggered(&env.task6.check_relay_output)) {
        t6_done(&env.task6.check_relay_output);

        // Fetch current status
        h = env.ct.h;
        m = env.ct.m;
        dow = env.dow;
        // Check events in all relays
        for (uint8_t j = 0; j < 3; j++) {
            for (uint8_t i = 0; i < env.config.relay[j].count &&
                i < NUM_EVENT_ENTRIES_PER_ITEM; i++) {
                port |= (event_output_state(
                    &env.config.relay[j].ev[i], h, m, dow) << j);
            }
        }
        // Set relay output
        PORTB = (PORTB & 0xf8) | (port << PORTB0);
    }
}

// T6: Queue serial output data and invoke transmission
void task6_serial_output() {
    if (t6_check_triggered(&env.task6.serial_output)) {
        t6_done(&env.task6.serial_output);

        // Clock date and time
        ringbuf_put(&tx, 'D');
        ringbuf_put(&tx, '0' + env.ct.yh);
        ringbuf_put(&tx, '0' + env.ct.yl);
        ringbuf_put(&tx, '-');
        ringbuf_put(&tx, '0' + env.ct.mo / 10);
        ringbuf_put(&tx, '0' + env.ct.mo % 10);
        ringbuf_put(&tx, '-');
        ringbuf_put(&tx, '0' + env.ct.d / 10);
        ringbuf_put(&tx, '0' + env.ct.d % 10);
        ringbuf_put(&tx, '\r');
        ringbuf_put(&tx, '\n');
        ringbuf_put(&tx, 'T');
        ringbuf_put(&tx, '0' + env.ct.h / 10);
        ringbuf_put(&tx, '0' + env.ct.h % 10);
        ringbuf_put(&tx, ':');
        ringbuf_put(&tx, '0' + env.ct.m / 10);
        ringbuf_put(&tx, '0' + env.ct.m % 10);
        ringbuf_put(&tx, ':');
        ringbuf_put(&tx, '0' + env.ct.s / 10);
        ringbuf_put(&tx, '0' + env.ct.s % 10);
        ringbuf_put(&tx, '\r');
        ringbuf_put(&tx, '\n');
        // Temperature
        ringbuf_put(&tx, 'A');
        if (env.temperature.result == 0) {
            ringbuf_put(&tx, env.temperature.value.sign ? '-' : '+');
            ringbuf_put(&tx, '0' + env.temperature.value.integer / 100);
            ringbuf_put(&tx, '0' + env.temperature.value.integer / 10 % 10);
            ringbuf_put(&tx, '0' + env.temperature.value.integer % 10);
            ringbuf_put(&tx, '.');
            ringbuf_put(&tx, '0' + env.temperature.value.fraction_x10k / 1000);
            ringbuf_put(&tx, '0' + env.temperature.value.fraction_x10k / 100 % 10);
        } else {
            ringbuf_put(&tx, ' ');
            ringbuf_put(&tx, 'x');
            ringbuf_put(&tx, 'x');
            ringbuf_put(&tx, 'x');
            ringbuf_put(&tx, '.');
            ringbuf_put(&tx, 'x');
            ringbuf_put(&tx, 'x');
        }
        ringbuf_put(&tx, '\r');
        ringbuf_put(&tx, '\n');
        // Enable interrupt to invoke transmission
        UCSR0B |= (1 << UDRIE0);
    }
}

// T9: Pick out received strings from buffer and parse them as NMEA sentence
void task9_handle_rx() {
    uint8_t c;
    zda_t zda;
    gga_t gga;
    
    while (!ringbuf_get(&rx, &c)) {
        linebuf_put(&env.msg, c);
        if (c == '\n') {
            if (strncmp((const char*) env.msg.data, "$GPGGA,", 7) == 0) {
                if (parse_gga(&gga, &(env.msg.data[7]),
                    env.msg.count - 7) == 0) {
                    env.gps.status = (gpstate_t) gga.status;
                    env.gps.sats_in_use = gga.sats_in_use;
                }
            }
            if (strncmp((const char*) env.msg.data, "$GPZDA,", 7) == 0) {
                if (parse_zda(&zda, &(env.msg.data[7]),
                    env.msg.count - 7) == 0) {
                    // Set acquired GPS time to clock if the position is fixed
                    if (env.config.use_gps && (env.gps.status == GP_GPS_FIX ||
                        env.gps.status == GP_DGPS_FIX)) {
                        env.ct = zda.ct;
                        env.dow = dayofweek(&env.ct);
                        // Trigger task as clock time is modified
                        t6_trigger(&env.task6.check_relay_output);
                    }
                }
            }
            linebuf_clear(&env.msg);
        }
    }
}

int main(void) {
    // -------- Setup --------
    
    // Initialize timing variables
    ticks = 0;
    env.ticks_rx = 0;

    // Initialize tasks
    t5_initialize(&env.task5.read_keys, T5_READ_KEYS_INTERVAL_MS);
    t5_initialize(&env.task5.read_temperature, T5_READ_TEMPERATURE_INTERVAL_MS);
    t5_initialize(&env.task5.draw_screen, T5_DRAW_SCREEN_INTERVAL_MS);
    t5_initialize(&env.task5.get_light_level, T5_GET_LIGHT_LEVEL_INTERVAL_MS);
    t6_initialize(&env.task6.save_ctime_to_rtc);
    t6_initialize(&env.task6.check_relay_output);
    t6_initialize(&env.task6.serial_output);

    // Initialize key watchers
    key_initialize(&env.key0);
    key_initialize(&env.key1);
    
    // Setup USART buffers
    ringbuf_initialize(&rx, RX_BUFFER_LENGTH);
    ringbuf_initialize(&tx, TX_BUFFER_LENGTH);
    linebuf_initialize(&env.msg, MESSAGE_BUFFER_LENGTH);

    // Initialize GPS status
    env.gps.status = GP_ABSENT;
    env.gps.sats_in_use = 0;
    
    // Setup SFRs
    setup_eeprom();
    setup_io();
    setup_timer0();
    setup_timer1();
    setup_usart0();
    setup_twi();
    setup_adc();

    // Enable all interrupts
    sei();
    
    // Restore configurations from EEPROM
    eeprom_redun_read((eeredun_t*) &eer_config, env.ee_blob);
    import_config_from_blob(&env.config, env.ee_blob);
    env.status = env.config.state_startup;

    // Setup temperature sensor
    env.temperature.result = 1;
    env.temperature.value.sign = 0;
    env.temperature.value.integer = 0;
    env.temperature.value.fraction_x10k = 0;
    env.temperature.flags = 0x00;
    temp_adt7410_set_config(
        CONFIG_ADT7410_FAULT_1 | CONFIG_ADT7410_POL_INTL_CTL |
        CONFIG_ADT7410_INTMODE_INT | CONFIG_ADT7410_OPMODE_CONT |
        CONFIG_ADT7410_RESOL_13BITS);
    // Read RTC on startup
    ctime_t ct_r;
    dow_t dow_r;
    bool success = false;
    for (uint8_t i = 0; i < RETRY_COUNT_READ_RTC_ON_STARTUP && !success; i++) {
        rtc_ds1307_read_clock(&ct_r, &dow_r);
        if (ctime_check_error(&ct_r) == 0x00) {
            // If any valid time has been read, set it to clock time
            env.ct = ct_r;
            env.dow = dayofweek((ctime_t*) &env.ct);
            success = true;
        }
        wait(2);
    }
    if (!success) {
        // On multiple attempts fail, load the default clock time
        env.ct = ct_default;
        env.dow = dayofweek((ctime_t*) &ct_default);
    }
    // Trigger relay output
    t6_trigger(&env.task6.check_relay_output);

    // -------- Loop --------

    while (true) {
        task5_read_keys();
        task5_read_temperature();
        task5_draw_screen();
        task5_set_brightness();
        task6_save_ctime_to_rtc();
        task6_check_relay_output();
        task6_serial_output();
        task9_handle_rx();
    }
}
