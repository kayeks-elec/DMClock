/*
 * eeprom_redundancy.h
 *
 *  Author: kayekss
 *  Target: unspecified
 */

#ifndef EEPROM_REDUNDANCY_H_
#define EEPROM_REDUNDANCY_H_

// EEPROM address map structure for wear-leveled writing operations
typedef struct {
    // EEPROM redundancy; when this value is N, that means:
    // ... requiring N times of storage for entities (+ N bytes for timestamps)
    // ... reducing write frequency per byte to 1/N
    uint8_t redundancy;
    // Address increment per block
    uint16_t stride;
    // The base addresses
    struct {
        // Of Timestamp
        uint16_t timestamp;
        // Of the entity
        uint16_t entity;
    } base;
} eeredun_t;

void eeprom_redun_initialize(eeredun_t* eer);
uint8_t eeprom_redun_pointer(eeredun_t* eer);
void eeprom_redun_read(eeredun_t* eer, uint8_t* blob);
void eeprom_redun_write(eeredun_t* eer, uint8_t* blob);

#endif
