#pragma once
#include "stm32f4xx_hal.h"
#include <stdint.h>

typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef      *cs_port;
    uint16_t           cs_pin;
    uint8_t            count;    // количество матриц в цепочке
} MAX7219_Handle_t;

void MAX7219_Init(MAX7219_Handle_t *dev);
void MAX7219_SetDigitAll(MAX7219_Handle_t *dev, uint8_t digit, uint8_t value);
void MAX7219_SetDigit(MAX7219_Handle_t *dev, uint8_t chip_idx, uint8_t digit, uint8_t value);
void MAX7219_SetBrightness(MAX7219_Handle_t *dev, uint8_t level); // 0..15
void MAX7219_Clear(MAX7219_Handle_t *dev);
void MAX7219_DrawMatrix(MAX7219_Handle_t *dev, uint8_t canvas[8][32]);
