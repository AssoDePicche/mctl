# MCTL

Interfacing via SPI an ESP32 running a WebServer with an STM32 to control a LED power

## Table of Contents

- [BOM](#bill-of-materials)
- [Libraries](#libraries)
- [Pinout](#pinout)

## Bill of Materials

| Item | Part Number | Description | Quantity |
| :--- | :--- | :--- | :--- |
| U1 | STM32F103C8T6 | MCU ARM Cortex-M3 72MHz | 1 |
| U2 | ESP32-DEV-KIT-V1 | MCU Xtensa 240MHz | 1 |
| D1 | NFP1315-51A | 0.96inch OLED Display | 1 |
| R1 | CFR-25JB-52-330 | 330/5% | 1 |
| RV1 | PTH902-030F-103B2 | POT 10K/5% | 1 |
| D1 | TLDR5800 | LED PTH 5mm Round Red Diffused | 1 |

## Libraries

- [olikraus/u8g2](https://github.com/olikraus/u8g2)

## Pinout

### ESP32-DEV-KIT-V1 Pinout

| Pin | Description |
| :--- | :--- |
| GPIO_5 | SPI CS |
| GPIO_18 | SPI SCKL |
| GPIO_19 | SPI MISO |
| GPIO_23 | SPI MOSI |


### STM32F103C8T6

| Pin | Description |
| :--- | :--- |
| PA0 | ADC_CH0 |
| PA4 | SPI CS |
| PA5 | SPI SCKL |
| PA6 | SPI MISO |
| PA7 | SPI MOSI |
| PA8 | TIM1_CH1 |
| PB6 | I2C SCL |
| PB7 | I2C SDA |
