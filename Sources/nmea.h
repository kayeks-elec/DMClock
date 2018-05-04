/*
 * nmea.h
 *
 *  Author: kayekss
 *  Target: unspecified
 */

#ifndef NMEA_H_
#define NMEA_H_

// Result structure gathered from $__GGA sentence
typedef struct {
    // UTC clock time
    ctime_t ct;
    // Latitude and longitude structures
    struct {
        // Direction of latitude/longitude
        // ... 'N' or 'S' for latitude, 'E' or 'W' for longitude
        uint8_t direction;
        // Integer part
        uint16_t integer;
        // Fraction part, multiplied by 10000
        uint16_t fraction;
    } latitude, longitude;
    // Position fix status
    uint8_t status;
    // Satellites in use
    uint8_t sats_in_use;
    // HDOP (Horizontal dilusion of precision)
    // Age of DGPS data in seconds
    struct {
        // Integer part
        uint16_t integer;
        // Fraction part, multiplied by 10000
        uint16_t fraction;
    } hdop, dgps_age;
    // MSL orthometric height
    // Geoid separation
    struct {
        // Sign (0: positive, 1: negative)
        bool sign;
        // Integer part
        uint8_t integer;
        // Fraction part, multiplied by 10000
        uint16_t fraction;
        // Units of value
        uint8_t units;
    } height, geoid_separation;
    // DGPS reference station ID
    uint16_t dgps_station_id;
    // Expected checksum value
    uint8_t checksum_expected;
    // Given checksum value
    uint8_t checksum_given;
} gga_t;

// Result structure gathered from $__ZDA sentence
typedef struct {
    // Localized clock time
    ctime_t ct;
    // Expected checksum value
    uint8_t checksum_expected;
    // Given checksum value
    uint8_t checksum_given;
} zda_t;

bool isdigitn(uint8_t* s, uint16_t n);
bool isxdigitn(uint8_t* s, uint16_t n);
uint16_t consecutive_digits(uint8_t* s);
uint8_t c2b(uint8_t c);
bool parse_gga(gga_t* gga, uint8_t* s, uint16_t count);
bool parse_zda(zda_t* zda, uint8_t* s, uint16_t count);

#endif
