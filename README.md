# DMCLOCK

## Overview

A Desktop clock project using AVR8 microcontroller and 32-by-16 dots LED matrix
module.

![Photo of a build](https://github.com/kayeks-elec/dmclock/blob/master/Assets/build.jpg "Photo of a build")

## Features
  - Display ambient temperature
  - Clock backup during power off
  - Four-level display brightness select, including automatic leveling with
    on-board ambient light sensor
  - Three-channel relay output triggerable by specified clock time and
    days-of-week.
  - Clock correction from GPS receiver
  - Serial message output (see below)

## Specifications
  - **Power**  
      - 5V DC from USB B connector, 250 mA typ., 800 mA max. including LED
        module
      - 3V DC from CR2032 cell, 10 nA typ. for clock backup
  - **Dimensions**  
    145 mm × 54 mm × 46 mm, viewing area 82 mm × 41 mm
  - **Devices**  
      - ATMEGA328P, clocked at 20 MHz
      - DS1307+ real-time clock
  - **Inputs**  
      - Two push switches
      - Ambient light sensor
      - Temperature sensor
      - GPS receiver module (optional)
  - **Outputs**  
      - Three relay channels with two throws each; max. load 2A 30V DC, 1A 125V
        AC
      - Serial message output: 5V level, 8N1 at 9600 baud
  - **Clock accuracy**  
    Less than 1 second of relative error from GPS clock source

## Targeting display device

The targeting LED matrix module in this project is Product Number of AD-501,
manufactured by *Alpha Device Co.* of Japan. It has a viewing area of
monochromatic 32-by-16 dots in matrix arrangement. Therefore, this project
would be portable to other devices which have similar viewing areas if you
replace the device-specific interfacing routines.

## Microcontroller

This project uses an ATMEGA328P microcontroller.

### Clock configuration

The clock is 20 MHz from crystal oscillator (ceramic resonator also works but not recommended).

### Flashing firmware

Flash firmware `Firmware/DotMatrixClock2018.hex` directly by ISP, with fuses
Low byte **0xFF**, High byte **0xD9**, and Extended byte **0x07**.

## GPS clock correction

External GPS modules can be connected to GPS Receiver connector (CN3) for clock
time correction.

The pin assignment for GPS Receiver connector (CN3):

| Pin # | Description |
| --: | :-- |
| 1 | +5V power supply, fused by 100 mA polyfuse |
| 2 | GND |
| 3 | not connected |
| 4 | GPS receiver source |
| 5 | GPS receiver sink |

Note:  
The data line through pins 4 and 5 constitutes a current loop driver, and
its driving current is 5 mA, which is similar to the conventional MIDI
interfaces.

Any GPS module which meets requirements below woule be available for this
project:
  - **Serial format**  
    8N1 at 9,600 baud
  - **Messages**  
    NMEA 0183 messages contain `$GPGGA` and `$GPZDA` sentences

## Serial message output

Messages are transmitted from Serial Output connector (CN4). The output level
is 5 V. Serial format is 8N1 at 9,600 baud.

### Message strings

#### Local clock date
```text
D([0-9][0-9])-([01][0-9])-([0-3][0-9])\r\n
  where \1: last two digits of years
        \2: months
        \3: days
```

#### Local clock time
```text
T([0-9][0-9]):([01][0-9]):([0-3][0-9])\r\n
  where \1: hours
        \2: minutes
        \3: seconds
```

#### Ambient temperature
```text
A([+-])([01][0-9][0-9].[0-9][0-9])\r\n
  if temperature is available
  where \1: sign of temperature
        \2: absolute value of temperature
A xxx.xx\r\n
  if temperature is unavailable
```

## License

Modified BSD License  
See `LICENSE` for details.
