/*
 * eeprom.h
 *
 *  Author: kayekss
 *  Target: ATmega328P
 */

#ifndef EEPROM_H_
#define EEPROM_H_

#define EEPROM_ADDRESS_MASK  0x03ff

uint8_t eeprom_read(uint16_t addr);
uint8_t eeprom_verify(uint16_t addr, uint8_t d);
void eeprom_write(uint16_t addr, uint8_t d, bool erase);
void eeprom_update(uint16_t addr, uint8_t d);

#endif
