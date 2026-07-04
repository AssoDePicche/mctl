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

- [stm32-ssd1306](https://github.com/afiskon/stm32-ssd1306)

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
| PA0 | ADC_IN0 |
| PA1 | TIM2_CH2 |
| PA4 | GPIO Output (SPI CS) |
| PA5 | SPI1_SCK |
| PA6 | SPI1_MISO |
| PA7 | SPI1_MOSI |
| PB6 | I2C1_SCL |
| PB7 | I2C1_SDA |
| PC13 | GPIO Output

- I2C1 Mode: I2C
- RCC HSE: Crystal/Ceramic Resonator
- SPI1 Mode: Full-Duplex Slave
- Hardware NSS Signal: Hardware NSS Input Signal
- PLLMul: x9 (72MHz)
- Prescaler: 0
- Counter Period: 4095
