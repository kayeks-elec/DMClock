/*
 * twi.c
 *
 *  Author: kayekss
 *  Target: ATmega328P
 */

#include <stdint.h>
#include <avr/io.h>
#include <util/twi.h>
#include "twi.h"

// Wait for TWI interrupt flag is set
inline void twi_wait_for_flag() {
    while (!(TWCR & (1 << TWINT)));
}

// Generate start condition, return 0 if successful
uint8_t twi_start_condition() {
    uint8_t result = 0;

    // Generate start condition
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    twi_wait_for_flag();
    // Check status code
    switch (TWSR & TW_STATUS_MASK) {
    case TW_START:  // Start condition transmitted
    case TW_REP_START:  // Repeated start condition transmitted
        result = 0;
        break;
    default:
        result = 1;
        break;
    }
    return result;
}

// Generate stop condition
void twi_stop_condition() {
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

// Transmit address
uint8_t twi_master_address(uint8_t addr) {
    uint8_t result = 0;
    
    // Set address to register
    TWDR = addr;
    TWCR = (1 << TWINT) | (1 << TWEN);
    twi_wait_for_flag();
    if (addr & 0x01) {
        // Master receiver mode
        switch (TWSR & TW_STATUS_MASK) {
        case TW_MR_SLA_ACK:  // SLA+R transmitted, ACK received
            result = 0;
            break;
        case TW_MR_SLA_NACK:  // SLA+R transmitted, NACK received
        default:
            result = 1;
            break;
        }
    } else {
        // Master transmitter mode
        switch (TWSR & TW_STATUS_MASK) {
        case TW_MT_SLA_ACK:  // SLA+W transmitted, ACK received
            result = 0;
            break;
        case TW_MT_SLA_NACK:  // SLA+W transmitted, NACK received
        default:
            result = 1;
            break;
        }
    }
    return result;
}

// Transmit data in Master Transmitter mode
uint8_t twi_master_transmit(uint8_t data) {
    uint8_t result = 0;

    // Set data to register
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    twi_wait_for_flag();
    // Check status code
    switch (TWSR & TW_STATUS_MASK) {
    case TW_MT_DATA_ACK:  // data transmitted, ACK received
        result = 0;
        break;
    case TW_MT_DATA_NACK:  // data transmitted, NACK received
    default:
        result = 1;
        break;
    }
    return result;
}

// Receive data in Master Receiver mode
uint8_t twi_master_receive(uint8_t* data, uint8_t ack) {
    uint8_t result = 0;

    // Set request to slave
    TWCR = (1 << TWINT) | (!!ack << TWEA) | (1 << TWEN);
    twi_wait_for_flag();
    // Check status code
    switch (TWSR & TW_STATUS_MASK) {
    case TW_MR_DATA_ACK:  // data received, ACK transmitted
    case TW_MR_DATA_NACK:  // data received, NACK transmitted
        *data = TWDR;
        result = 0;
        break;
    default:
        result = 1;
        break;
    }
    return result;
}
