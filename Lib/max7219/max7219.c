#include "max7219.h"

// Регистры MAX7219
#define REG_NOOP        0x00
#define REG_DIGIT(n)   (0x01 + (n))   // 0..7
#define REG_DECODE_MODE 0x09
#define REG_INTENSITY   0x0A
#define REG_SCAN_LIMIT  0x0B
#define REG_SHUTDOWN    0x0C
#define REG_DISPLAY_TEST 0x0F


static void write_reg(MAX7219_Handle_t *dev, uint8_t reg, uint8_t data) {
    uint8_t buf[2] = {reg, data};
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(dev->hspi, buf, 2, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
}

// Отправить одну команду нужному чипу, остальным — NOOP
static void write_reg_chain(MAX7219_Handle_t *dev, uint8_t chip_idx,
                             uint8_t reg, uint8_t data)
{
    // Буфер на всю цепочку
    uint8_t buf[dev->count * 2];

    for (uint8_t i = 0; i < dev->count; i++) {
        // Данные идут в обратном порядке: последний чип — первым в буфере
        if (i == (dev->count - 1 - chip_idx)) {
            buf[i * 2]     = reg;
            buf[i * 2 + 1] = data;
        } else {
            buf[i * 2]     = REG_NOOP;  // 0x00
            buf[i * 2 + 1] = 0x00;
        }
    }

    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(dev->hspi, buf, dev->count * 2, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
}

static void write_reg_all(MAX7219_Handle_t *dev, uint8_t reg, uint8_t data) {
    uint8_t buf[dev->count * 2];

    for (uint8_t i = 0; i < dev->count; i++) {
        buf[i * 2]     = reg;
        buf[i * 2 + 1] = data;
    }

    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(dev->hspi, buf, dev->count * 2, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
}

void MAX7219_Init(MAX7219_Handle_t *dev) {
    // Каждую команду нужно разослать всем чипам
    write_reg_all(dev, REG_SHUTDOWN,    0x01); // выходим из shutdown
    write_reg_all(dev, REG_DISPLAY_TEST, 0x00); // тест выкл
    write_reg_all(dev, REG_DECODE_MODE, 0x00); // без BCD-декодирования
    write_reg_all(dev, REG_SCAN_LIMIT,  0x07); // все 8 цифр/строк
    write_reg_all(dev, REG_INTENSITY,   0x08); // яркость по умолчанию
    MAX7219_Clear(dev);
}

void MAX7219_Clear(MAX7219_Handle_t *dev) {
    for (uint8_t row = 0; row < 8; row++)
        write_reg_all(dev, REG_DIGIT(row), 0x00);
}

void MAX7219_SetDigitAll(MAX7219_Handle_t *dev, uint8_t digit, uint8_t value) {
    write_reg_all(dev, REG_DIGIT(digit), value);
}

void MAX7219_SetDigit(MAX7219_Handle_t *dev, uint8_t chip_idx, uint8_t digit, uint8_t value) {
    write_reg_chain(dev, chip_idx, REG_DIGIT(digit), value);
}

void MAX7219_SetBrightness(MAX7219_Handle_t *dev, uint8_t level) {
    write_reg_all(dev, REG_INTENSITY, level & 0x0F);
}

void MAX7219_DrawMatrix(MAX7219_Handle_t *dev, uint8_t canvas[8][32])
{
    // Для каждой строки
    for (uint8_t y = 0; y < 8; y++) {
        uint8_t row = 7 - y;  // переворот по вертикали

        // Для каждого чипа
        for (uint8_t chip = 0; chip < dev->count; chip++) {
            uint8_t byte = 0;

            // Собираем байт из 8 пикселей этой строки
            for (uint8_t bit = 0; bit < 8; bit++) {
                uint8_t x = chip * 8 + bit;

                if (canvas[y][x]) {
                    // Переворот по горизонтали: бит 0 — правый
                    byte |= (1 << bit);
                }
            }

            write_reg_chain(dev, chip, REG_DIGIT(row), byte);
        }
    }
}
