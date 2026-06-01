#ifndef MPU6050_H
#define MPU6050_H

#include "stm32f4xx_hal.h"

/* I2C адрес (AD0=GND → 0x68, сдвигаем влево для HAL) */
#define MPU6050_ADDR_LOW   (0x68 << 1)   /* AD0 = GND */
#define MPU6050_ADDR_HIGH  (0x69 << 1)   /* AD0 = 3.3V */
#define MPU6050_ADDR       MPU6050_ADDR_LOW

/* Основные регистры */
#define MPU6050_REG_SMPLRT_DIV    0x19
#define MPU6050_REG_CONFIG        0x1A
#define MPU6050_REG_GYRO_CONFIG   0x1B
#define MPU6050_REG_ACCEL_CONFIG  0x1C
#define MPU6050_REG_INT_ENABLE    0x38
#define MPU6050_REG_ACCEL_XOUT_H  0x3B   /* начало блока: AX,AY,AZ,TEMP,GX,GY,GZ */
#define MPU6050_REG_TEMP_OUT_H    0x41
#define MPU6050_REG_GYRO_XOUT_H   0x43
#define MPU6050_REG_PWR_MGMT_1    0x6B
#define MPU6050_REG_WHO_AM_I      0x75

/* Диапазоны акселерометра (REG_ACCEL_CONFIG, биты AFS_SEL) */
#define MPU6050_ACCEL_2G   0x00   /* ±2g,  LSB = 16384 */
#define MPU6050_ACCEL_4G   0x08   /* ±4g,  LSB = 8192  */
#define MPU6050_ACCEL_8G   0x10   /* ±8g,  LSB = 4096  */
#define MPU6050_ACCEL_16G  0x18   /* ±16g, LSB = 2048  */

/* Диапазоны гироскопа (REG_GYRO_CONFIG, биты FS_SEL) */
#define MPU6050_GYRO_250   0x00   /* ±250  °/s, LSB = 131.0  */
#define MPU6050_GYRO_500   0x08   /* ±500  °/s, LSB = 65.5   */
#define MPU6050_GYRO_1000  0x10   /* ±1000 °/s, LSB = 32.8   */
#define MPU6050_GYRO_2000  0x18   /* ±2000 °/s, LSB = 16.4   */

/* Масштабные коэффициенты для диапазонов по умолчанию */
#define MPU6050_ACCEL_SCALE  16384.0f   /* для ±2g  */
#define MPU6050_GYRO_SCALE   131.0f     /* для ±250°/s */
#define MPU6050_TEMP_SCALE   340.0f
#define MPU6050_TEMP_OFFSET  36.53f

/* Структуры данных */
typedef struct {
    float x, y, z;   /* единицы: g */
} MPU6050_Accel;

typedef struct {
    float x, y, z;   /* единицы: °/с */
} MPU6050_Gyro;

typedef struct {
    MPU6050_Accel accel;
    MPU6050_Gyro  gyro;
    float         temp_c;   /* температура в °C */
} MPU6050_Data;

/* Коды ошибок */
typedef enum {
    MPU6050_OK    = 0,
    MPU6050_ERR_I2C,
    MPU6050_ERR_WHO_AM_I,
} MPU6050_Status;

/* API */
MPU6050_Status MPU6050_Init(I2C_HandleTypeDef *hi2c);
MPU6050_Status MPU6050_ReadAll(I2C_HandleTypeDef *hi2c, MPU6050_Data *data);
MPU6050_Status MPU6050_ReadAccel(I2C_HandleTypeDef *hi2c, MPU6050_Accel *accel);
MPU6050_Status MPU6050_ReadGyro(I2C_HandleTypeDef *hi2c, MPU6050_Gyro *gyro);
MPU6050_Status MPU6050_ReadTemp(I2C_HandleTypeDef *hi2c, float *temp_c);

#endif /* MPU6050_H */
