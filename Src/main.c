/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "max7219.h"
#include "mpu6050.h"
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
    float x, y;   // непрерывная позиция
    float vx, vy; // скорость
} Player;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define CANVAS_ROWS     8
#define CANVAS_COLS     32
#define PLAYER_MIN_X    1
#define PLAYER_MAX_X    (CANVAS_COLS - 2)
#define PLAYER_MIN_Y    1
#define PLAYER_MAX_Y    (CANVAS_ROWS - 2)

#define IMU_DEADZONE    0.05f   // меньше дедзона — чувствительнее
#define ACCEL_X_OFFSET  0.01f
#define ACCEL_Y_OFFSET -0.03f
#define ACCEL_Z_OFFSET  0.24f

#define GRAVITY_SCALE   0.16f
#define FRICTION        0.90f   // чуть меньше трения для заметного отскока
#define MAX_VELOCITY    2.0f    // было 0.8f — было слишком мало для отскока!
#define BOUNCE          0.85f   // упругость стенок (0..1)

#define TICK_MS         25      // период обновления физики, мс
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN PV */
MPU6050_Data   imu;
MPU6050_Status imu_status;

MAX7219_Handle_t display = {
    .hspi    = &hspi1,
    .cs_port = GPIOA,
    .cs_pin  = GPIO_PIN_4,
    .count   = 4,
};

static uint8_t canvas[CANVAS_ROWS][CANVAS_COLS];
static uint32_t last_tick = 0;
static Player  player;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* Переинициализация I2C + IMU после ошибки шины */
static void IMU_Reinit(void)
{
    HAL_I2C_DeInit(&hi2c1);
    HAL_Delay(10);          // дать шине осесть
    HAL_I2C_Init(&hi2c1);
    imu_status = MPU6050_Init(&hi2c1);
}

static float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void Physics_Update(float ax, float ay)
{
  // Применяем дедзону
  float gx = (fabsf(ax) > IMU_DEADZONE) ? (ax - ACCEL_X_OFFSET) : 0.0f;
  float gy = (fabsf(ay) > IMU_DEADZONE) ? (ay - ACCEL_Y_OFFSET) : 0.0f;

  // Накапливаем скорость
  player.vx += gx * GRAVITY_SCALE;
  player.vy += gy * GRAVITY_SCALE;

  // Ограничиваем максимальную скорость
  player.vx = clampf(player.vx, -MAX_VELOCITY, MAX_VELOCITY);
  player.vy = clampf(player.vy, -MAX_VELOCITY, MAX_VELOCITY);

  // Новая позиция
  float nx = player.x + player.vx;
  float ny = player.y + player.vy;

  // Коллизия ПЕРЕД трением — иначе отскок гасится в том же тике
  if (nx < PLAYER_MIN_X) { nx = PLAYER_MIN_X; player.vx = fabsf(player.vx)  * BOUNCE; }
  if (nx > PLAYER_MAX_X) { nx = PLAYER_MAX_X; player.vx = -fabsf(player.vx) * BOUNCE; }
  if (ny < PLAYER_MIN_Y) { ny = PLAYER_MIN_Y; player.vy = fabsf(player.vy)  * BOUNCE; }
  if (ny > PLAYER_MAX_Y) { ny = PLAYER_MAX_Y; player.vy = -fabsf(player.vy) * BOUNCE; }

  // Трение ПОСЛЕ отскока
  player.vx *= FRICTION;
  player.vy *= FRICTION;

  // Обновляем canvas только если пиксель реально сдвинулся
  uint8_t px = (uint8_t)(nx + 0.5f);
  uint8_t py = (uint8_t)(ny + 0.5f);

  if (px != (uint8_t)(player.x + 0.5f) || py != (uint8_t)(player.y + 0.5f))
  {
      canvas[(uint8_t)(player.y + 0.5f)][(uint8_t)(player.x + 0.5f)] = 0;
      canvas[py][px] = 1;
      MAX7219_DrawMatrix(&display, canvas);
  }

  player.x = nx;
  player.y = ny;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  MAX7219_Init(&display);
  MAX7219_SetBrightness(&display, 0);

  /* Нарисовать борты */
  for (uint8_t y = 0; y < CANVAS_ROWS; y++) {
      canvas[y][0]             = 1;
      canvas[y][CANVAS_COLS-1] = 1;
  }
  for (uint8_t x = 0; x < CANVAS_COLS; x++) {
      canvas[0][x]             = 1;
      canvas[CANVAS_ROWS-1][x] = 1;
  }

  /* Начальная позиция игрока */
  player.x  = CANVAS_COLS / 2;
  player.y  = CANVAS_ROWS / 2;
  player.vx = 0;
  player.vy = 0;
  canvas[(uint8_t)player.y][(uint8_t)player.x] = 1;
  MAX7219_DrawMatrix(&display, canvas);

  imu_status = MPU6050_Init(&hi2c1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // Неблокирующий таймер вместо HAL_Delay
    if (HAL_GetTick() - last_tick < TICK_MS)
      continue;
    last_tick = HAL_GetTick();

    if (imu_status == MPU6050_OK) {
      if (HAL_I2C_GetState(&hi2c1) == HAL_I2C_STATE_READY)
      imu_status = MPU6050_ReadAll(&hi2c1, &imu);
    }

    if (imu_status == MPU6050_OK)
      Physics_Update(-imu.accel.x, -imu.accel.y);
    else
      IMU_Reinit();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(MAX7219_CS_GPIO_Port, MAX7219_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : MAX7219_CS_Pin */
  GPIO_InitStruct.Pin = MAX7219_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(MAX7219_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PB12 PB13 PB14 PB15 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {

  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
