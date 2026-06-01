#include "mpu6050.h"

/* ------------------------------------------------------------------ */
/* Вспомогательные функции                                              */
/* ------------------------------------------------------------------ */

static HAL_StatusTypeDef write_reg(I2C_HandleTypeDef *hi2c,
                                   uint8_t reg, uint8_t val)
{
    return HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, reg,
                             I2C_MEMADD_SIZE_8BIT, &val, 1, 10);
}

static HAL_StatusTypeDef read_regs(I2C_HandleTypeDef *hi2c,
                                   uint8_t reg, uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, reg,
                            I2C_MEMADD_SIZE_8BIT, buf, len, 10);
}

/* Склейка двух байт в знаковое 16-битное число */
static inline int16_t combine(uint8_t hi, uint8_t lo)
{
    return (int16_t)((hi << 8) | lo);
}

/* ------------------------------------------------------------------ */
/* Инициализация                                                        */
/* ------------------------------------------------------------------ */

MPU6050_Status MPU6050_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t who = 0;

    /* Проверка WHO_AM_I — должен вернуть 0x68 */
    // if (read_regs(hi2c, MPU6050_REG_WHO_AM_I, &who, 1) != HAL_OK)
    //     return MPU6050_ERR_I2C;
    // if (who != 0x70)
    //     return MPU6050_ERR_WHO_AM_I;

    /* Снять с sleep: PWR_MGMT_1 = 0x00, тактирование от внутр. 8МГц */
    if (write_reg(hi2c, MPU6050_REG_PWR_MGMT_1, 0x00) != HAL_OK)
        return MPU6050_ERR_I2C;

    /* Sample Rate Divider: 0 → частота дискретизации = 1 кГц / (1+0) = 1 кГц */
    if (write_reg(hi2c, MPU6050_REG_SMPLRT_DIV, 0x00) != HAL_OK)
        return MPU6050_ERR_I2C;

    /* DLPF (Digital Low Pass Filter): bandwidth ≈ 260 Гц, delay 0 мс */
    if (write_reg(hi2c, MPU6050_REG_CONFIG, 0x00) != HAL_OK)
        return MPU6050_ERR_I2C;

    /* Гироскоп: ±250 °/с */
    if (write_reg(hi2c, MPU6050_REG_GYRO_CONFIG, MPU6050_GYRO_250) != HAL_OK)
        return MPU6050_ERR_I2C;

    /* Акселерометр: ±2g */
    if (write_reg(hi2c, MPU6050_REG_ACCEL_CONFIG, MPU6050_ACCEL_2G) != HAL_OK)
        return MPU6050_ERR_I2C;

    return MPU6050_OK;
}

/* ------------------------------------------------------------------ */
/* Чтение всех данных за один I2C-транзакцией (14 байт)                */
/* ------------------------------------------------------------------ */

MPU6050_Status MPU6050_ReadAll(I2C_HandleTypeDef *hi2c, MPU6050_Data *data)
{
    uint8_t buf[14];

    /* Читаем блок с 0x3B: ACCEL(6) + TEMP(2) + GYRO(6) */
    if (read_regs(hi2c, MPU6050_REG_ACCEL_XOUT_H, buf, 14) != HAL_OK)
        return MPU6050_ERR_I2C;

    /* Акселерометр */
    data->accel.x = (float)combine(buf[0],  buf[1])  / MPU6050_ACCEL_SCALE;
    data->accel.y = (float)combine(buf[2],  buf[3])  / MPU6050_ACCEL_SCALE;
    data->accel.z = (float)combine(buf[4],  buf[5])  / MPU6050_ACCEL_SCALE;

    /* Температура: T[°C] = raw / 340 + 36.53 */
    int16_t raw_temp = combine(buf[6], buf[7]);
    data->temp_c = (float)raw_temp / MPU6050_TEMP_SCALE + MPU6050_TEMP_OFFSET;

    /* Гироскоп */
    data->gyro.x  = (float)combine(buf[8],  buf[9])  / MPU6050_GYRO_SCALE;
    data->gyro.y  = (float)combine(buf[10], buf[11]) / MPU6050_GYRO_SCALE;
    data->gyro.z  = (float)combine(buf[12], buf[13]) / MPU6050_GYRO_SCALE;

    return MPU6050_OK;
}

/* ------------------------------------------------------------------ */
/* Отдельное чтение акселерометра (6 байт)                             */
/* ------------------------------------------------------------------ */

MPU6050_Status MPU6050_ReadAccel(I2C_HandleTypeDef *hi2c, MPU6050_Accel *accel)
{
    uint8_t buf[6];
    if (read_regs(hi2c, MPU6050_REG_ACCEL_XOUT_H, buf, 6) != HAL_OK)
        return MPU6050_ERR_I2C;

    accel->x = (float)combine(buf[0], buf[1]) / MPU6050_ACCEL_SCALE;
    accel->y = (float)combine(buf[2], buf[3]) / MPU6050_ACCEL_SCALE;
    accel->z = (float)combine(buf[4], buf[5]) / MPU6050_ACCEL_SCALE;
    return MPU6050_OK;
}

/* ------------------------------------------------------------------ */
/* Отдельное чтение гироскопа (6 байт)                                 */
/* ------------------------------------------------------------------ */

MPU6050_Status MPU6050_ReadGyro(I2C_HandleTypeDef *hi2c, MPU6050_Gyro *gyro)
{
    uint8_t buf[6];
    if (read_regs(hi2c, MPU6050_REG_GYRO_XOUT_H, buf, 6) != HAL_OK)
        return MPU6050_ERR_I2C;

    gyro->x = (float)combine(buf[0], buf[1]) / MPU6050_GYRO_SCALE;
    gyro->y = (float)combine(buf[2], buf[3]) / MPU6050_GYRO_SCALE;
    gyro->z = (float)combine(buf[4], buf[5]) / MPU6050_GYRO_SCALE;
    return MPU6050_OK;
}

/* ------------------------------------------------------------------ */
/* Отдельное чтение температуры (2 байта)                              */
/* ------------------------------------------------------------------ */

MPU6050_Status MPU6050_ReadTemp(I2C_HandleTypeDef *hi2c, float *temp_c)
{
    uint8_t buf[2];
    if (read_regs(hi2c, MPU6050_REG_TEMP_OUT_H, buf, 2) != HAL_OK)
        return MPU6050_ERR_I2C;

    int16_t raw = combine(buf[0], buf[1]);
    *temp_c = (float)raw / MPU6050_TEMP_SCALE + MPU6050_TEMP_OFFSET;
    return MPU6050_OK;
}
