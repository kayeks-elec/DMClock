/*
 * eeprom_redundancy.c
 *
 *  Author: kayekss
 *  Target: unspecified
 */

#include <stdbool.h>
#include <stdint.h>
#include "eeprom.h"
#include "eeprom_redundancy.h"

// Initialize timestamp segment
void eeprom_redun_initialize(eeredun_t* eer) {
    for (uint8_t i = 0; i < eer->redundancy; i++) {
        eeprom_update(eer->base.timestamp + i, 0);
    }
}

// Find index for next read by scanning timestamp segment
uint8_t eeprom_redun_pointer(eeredun_t* eer) {
    uint8_t result = 0;
    uint8_t ts = ~0;
    uint8_t ts_next = ~0;
    
    for (uint8_t i = 0; i < eer->redundancy; i++) {
        uint8_t i_next = i >= eer->redundancy - 1 ? 0 : i + 1;
        ts = eeprom_read(eer->base.timestamp + i);
        ts_next = eeprom_read(eer->base.timestamp + i_next);
        
        if ((uint8_t) (ts + 1) != ts_next) {
            result = i;
            break;
        }
    }
    return result;
}

// Read entity from EEPROM block with redundancy
void eeprom_redun_read(eeredun_t* eer, uint8_t* blob) {
    uint8_t p = eeprom_redun_pointer(eer);
    uint16_t base = eer->base.entity + eer->stride * p;
    
    for (uint16_t i = 0; i < eer->stride; i++) {
        blob[i] = eeprom_read(base + i);
    }
}

// Write entity from EEPROM block with redundancy
void eeprom_redun_write(eeredun_t* eer, uint8_t* blob) {
    uint8_t p = eeprom_redun_pointer(eer);
    uint8_t wp = p >= eer->redundancy - 1 ? 0 : p + 1;
    uint16_t base_w = eer->base.entity + eer->stride * wp;
    
    // Write entity blob
    for (uint16_t i = 0; i < eer->stride; i++) {
        eeprom_update(base_w + i, blob[i]);
    }
    // Write timestamp
    eeprom_update(eer->base.timestamp + wp,
        eeprom_read(eer->base.timestamp + p) + 1);
}
