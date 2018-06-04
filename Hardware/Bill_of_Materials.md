# Bill of Materials

## Mainboard

| Nr. | Location | Category | Details | Qty. |
|----:|:---------|:---------|:--------|-----:|
| 1 | R4 | carbon film resistor | 220 ohm | 1 |
| 2 | R1, R3, R5, R7, R9, R11 | carbon film resistor | 1K ohm | 6 |
| 3 | R12 | carbon film resistor | 2.2K ohm (depending on characteristics of RP) | 1 |
| 4 | R6, R8, R10 | carbon film resistor | 4.7K ohm | 3 |
| 5 | R2, R13, R14 | carbon film resistor | 10K ohm | 3 |
| 6 | RP | photocell | 10K - 20K ohm @ 10 lux, 1M ohm @ dark | 1 |
| 7 | F1 | resettable fuse | ≥10 V, 1.1 A | 1 |
| 8 | F2 | resettable fuse | ≥10 V, 0.1 A | 1 |
| 9 | C7, C8 | ceramic capacitor | 10 pF (depending on characteristics of Y2) | 2 |
| 10 | C4, C5 | ceramic capacitor | 22 pF (depending on characteristics of Y1) | 2 |
| 11 | C2, C3, C6, C9 | multilayer ceramic capacitor | 100 nF | 4 |
| 12 | C1 | electrolytic capacitor | ≥10 V, 220 uF | 1 |
| 13 | Y1 | crystal oscillator | 20.000 MHz | 1 |
| 14 | Y2 | crystal oscillator | 32.768 KHz | 1 |
| 15 | D1, D2, D3, D4 | rectifier diode | *1N4148* | 4 |
| 16 | LD1 | light emitting diode | *red* | 1 |
| 17 | LD2 | light emitting diode | *yellow green* | 1 |
| 18 | LD3, LD4, LD5 | light emitting diode | *yellow* | 3 |
| 19 | Q1, Q2, Q4, Q5, Q6 | NPN transistor | *2N3904* | 5 |
| 20 | Q3 | optoisolator | *PC817* | 1 |
| 21 | U1 | AVR8 microcontroller | ATMEGA328P-PU, `Firmware/DotMatrixClock2018.hex` programmed, fuses H=0xFF L=0xD9 E=0x07 | 1 |
| 22 | U2 | real-time clock | DS1307+ | 1 |
| 23 | U3 | digital temperature sensor | ADT7410TRZ | 1 |
| 24 | CN1 | USB B connector | | 1 |
| 25 | CN2 | JST PH connector | B10B-PH-K-S | 1 |
| 26 | CN3 | DIN connector | 5 poles | 1 |
| 27 | CN4 | pin header | 6 poles, single row, right angle | 1 |
| 28 | CN5, CN6, CN7 | solderless terminal block | 3 poles | 3 |
| 29 | CN8 | box header | 6 poles | 1 |
| 30 | BATT | CR2032 coin cell holder | | 1 |
| 31 | SW1, SW2 | push switch | | 2 |
| 32 | RY1, RY2, RY3 | magnetic relay | 5 V, double throw, *946H-1C-5D* | 3 |
| 33 | - | LED panel module | AD-501 from Alpha Device Co. | 1 |

## GPS Receiver

| Nr. | Location | Category | Details | Qty. |
|----:|:---------|:---------|:--------|-----:|
| 1 | R1, R2 | carbon film resistor | 220 ohm | 2 |
| 2 | R3, R4 | carbon film resistor | 1K ohm | 2 |
| 3 | C1 | electrolytic capacitor | ≥10 V, 22 uF | 1 |
| 4 | U1 | GPS module | AE-GYSFDMAX from Akizuki Denshi | 2 |
| 5 | (U1) | pin header | 5 poles, single row | 1 |
| 6 | (U1) | pin socket | 5 poles, single row | 1 |
| 7 | (U1) | standoff | M3, 11mm, brass | 4 |
| 8 | (U1) | screw | M3, 5mm (+), steel | 8 |
| 9 | CN1 | DIN connector | 5 poles | 1 |
| 10 | CN2 | pin header | 6 poles, single row, right angle | 1 |
| 11 | CN3 | pin header | 2 poles, single row, right angle | 1 |

  - P/Ns described in italic are reference designs. You can replace them to
    compatible parts as your availability.
