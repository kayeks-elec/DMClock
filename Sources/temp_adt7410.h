/*
 * DotMatrixClock2018/temp_adt7410.h
 *
 *  Author: kayekss
 *  Target: ATmega328P, 20.000 MHz crystal oscillator
 */

#ifndef TEMP_ADT7410_H_
#define TEMP_ADT7410_H_

// TWI slave address
#define ADDR7_ADT7410  0x48
#define ADDRR_ADT7410  ((ADDR7_ADT7410 << 1) + 0x01)
#define ADDRW_ADT7410  (ADDR7_ADT7410 << 1)

// Enumeration table of register address
typedef enum {
    REG_ADT7410_VALUE_MSB         = 0x00,
    REG_ADT7410_VALUE_LSB         = 0x01,
    REG_ADT7410_STATUS            = 0x02,
    REG_ADT7410_CONFIG            = 0x03,
    REG_ADT7410_SETPOINT_HIGH_MSB = 0x04,
    REG_ADT7410_SETPOINT_HIGH_LSB = 0x05,
    REG_ADT7410_SETPOINT_LOW_MSB  = 0x06,
    REG_ADT7410_SETPOINT_LOW_LSB  = 0x07,
    REG_ADT7410_SETPOINT_CRIT_MSB = 0x08,
    REG_ADT7410_SETPOINT_CRIT_LSB = 0x09,
    REG_ADT7410_SETPOINT_HYST     = 0x0a,
    REG_ADT7410_ID                = 0x0b,
    REG_ADT7410_RESET             = 0x2f
} reg_adt7410_t;
// Enumeration tables of config bits
typedef enum {
    CONFIG_ADT7410_FAULT_1 = 0x00,
    CONFIG_ADT7410_FAULT_2 = 0x01,
    CONFIG_ADT7410_FAULT_3 = 0x02,
    CONFIG_ADT7410_FAULT_4 = 0x03
} config_ad7410fault_t;
typedef enum {
    CONFIG_ADT7410_POL_INTL_CTL = 0x00,
    CONFIG_ADT7410_POL_INTL_CTH = 0x04,
    CONFIG_ADT7410_POL_INTH_CTL = 0x08,
    CONFIG_ADT7410_POL_INTH_CTH = 0x0c
} config_ad7410pol_t;
typedef enum {
    CONFIG_ADT7410_INTMODE_INT = 0x00,
    CONFIG_ADT7410_INTMODE_CT  = 0x10
} config_ad7410intmode_t;
typedef enum {
    CONFIG_ADT7410_OPMODE_CONT     = 0x00,
    CONFIG_ADT7410_OPMODE_ONESHOT  = 0x20,
    CONFIG_ADT7410_OPMODE_1SPS     = 0x40,
    CONFIG_ADT7410_OPMODE_SHUTDOWN = 0x60
} config_ad7410opmode_t;
typedef enum {
    CONFIG_ADT7410_RESOL_13BITS = 0x00,
    CONFIG_ADT7410_RESOL_16BITS = 0x80,
} config_ad7410resol_t;

// Temperature result structure
typedef struct {
    // Sign (0: positive/zero, 1: negative)
    bool sign;
    // Absolute values separated in integer and fraction
    uint8_t integer;
    uint16_t fraction_x10k;
} temp_adt7410_t;

uint8_t temp_adt7410_set_config(uint8_t config);
uint8_t temp_adt7410_read_temperature(temp_adt7410_t* value, uint8_t* flags,
    config_ad7410resol_t resol);

#endif
