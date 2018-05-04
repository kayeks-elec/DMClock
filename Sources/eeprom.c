/*
 * eeprom.c
 *
 *  Author: kayekss
 *  Target: ATmega328P
 */

#include <stdbool.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "eeprom.h"

// Read a byte from EEPROM
uint8_t eeprom_read(uint16_t addr) {
    // Wait until EEPROM is ready
    while (EECR & (1 << EEPE));
    // Set address to read
    EEAR = addr & EEPROM_ADDRESS_MASK;
    // Start EEPROM read; data will be ready immediately
    EECR |= (1 << EERE);
    return EEDR;
}

// Verify a byte in EEPROM
// ... return XOR of read value and expected value
// ... of ~0 at read failure
uint8_t eeprom_verify(uint16_t addr, uint8_t d) {
    uint8_t result;
    
    if (addr & ~EEPROM_ADDRESS_MASK) {
        // Return ones when the address is out of range
        result = ~0;
    } else {
        result = eeprom_read(addr) ^ d;
    }
    return result;
}

// Write a byte to EEPROM
void eeprom_write(uint16_t addr, uint8_t d, bool erase) {
    // Wait until EEPROM is ready
    while (EECR & (1 << EEPE));
    // while (SPMCSR & (1 << SPMEN));
    cli();
    // Set programming mode
    EECR = (EECR & 0x0f) | ((erase ? 0b00 : 0b10) << EEPM0);
    // Set address and data to write
    EEAR = addr & EEPROM_ADDRESS_MASK;
    EEDR = d;
    // Start EEPROM write by setting EEMPE and EEPE bit
    // ... in the order described
    EECR |= (1 << EEMPE);
    EECR |= (1 << EEPE);
    sei();
    // Wait until EEPROM finishes writing
    while (EECR & (1 << EEPE));
}

// Update a byte to EEPROM
void eeprom_update(uint16_t addr, uint8_t d) {
    uint8_t r;
    
    r = eeprom_read(addr);
    if (r != d) {
        if (~r & d) {
            // Erase-and-write if there is any bit in the byte is
            // ... not overwritable (0 -> 1)
            eeprom_write(addr, d, true);
        } else {
            // Overwrite if all bits in the byte are overwritable
            // ... (1 -> 1) (1 -> 0) (0 -> 0)
            eeprom_write(addr, d, false);
        }
    }
}
