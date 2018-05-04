/*
 * twi.h
 *
 *  Author: kayekss
 *  Target: ATmega328P
 */

#ifndef TWI_H_
#define TWI_H_

#define TWI_ACK   1
#define TWI_NACK  0

void twi_wait_for_flag();
uint8_t twi_start_condition();
void twi_stop_condition();
uint8_t twi_master_address(uint8_t addr);
uint8_t twi_master_transmit(uint8_t data);
uint8_t twi_master_receive(uint8_t* data, uint8_t ack);

#endif
