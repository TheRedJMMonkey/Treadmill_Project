/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
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
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "I2C_LCD_PCF8574.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

static const uint8_t SRAM_WR_CMD = 0x02;
static const uint8_t SRAM_RD_CMD = 0x03;

static const int STEPS_PER_REV = 2048;

#define MAX_SAVE_SLOTS 10
#define WORKOUT_ADDR_BASE 0x0000

#define WORKOUT_MAGIC 0xAA55
#define WORKOUT_SIZE sizeof(WorkoutData_t)

#define MENU_LIVE_DISPLAY 0
#define MENU_WRITE_WORKOUT 1
#define MENU_VIEW_HISTORY 2
#define MENU_SETTINGS 3

typedef struct
{
  uint16_t magicNumber;
  int32_t totalSteps;
  int32_t totalDistance;
  int32_t totalCalories;
  uint16_t slotUsed;
} __packed WorkoutData_t;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim7;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
int totalSteps = 0;
int totalDistance = 0; // meters
int totalCalories = 0;
volatile int encDir = 0;
volatile int step = 0;
int stepDir = 0;
int targetStepperRPM = 0;
int targetServoPosDeg = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI2_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM7_Init(void);
static void MX_TIM1_Init(void);
/* USER CODE BEGIN PFP */

int distanceTraveled(int motorSteps);

int stepsWalked(int distanceTraveled);

int caloriesBurned(int stepsWalked);

void readSRAM(uint16_t addr, uint8_t *rd_array, uint16_t size);

void writeSRAM(uint16_t addr, uint8_t *wr_array, uint16_t size);

void setServoPos(double servoPosDeg);

void setStepperSpeed(double rpm, int direction);

void adjustSettings(void);

void updateLiveDisplay(void);

void saveWorkout(uint8_t slot);

void loadWorkout(uint8_t slot);

void displayWorkoutHistory(void);

void processUARTCommand(uint8_t *cmdBuffer);

void handleViewWorkouts(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

  /* USER CODE BEGIN 1 */
  char lcdStr1[17];
  char lcdStr2[17];
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
  MX_I2C1_Init();
  MX_SPI2_Init();
  MX_USART1_UART_Init();
  MX_TIM7_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  // Start timer 7 for the stepper
  HAL_TIM_Base_Start_IT(&htim7);

  // Start 40Hz PWM for the servo
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
  TIM1->CCR2 = 0;

  // Initalize the LCD
  LCD_Start();

  // Configure SRAM for sequential reads and writes
  uint8_t sramEnSequentialReadCMD[2] = {0x05, 0x40};
  uint8_t sramEnSequentialWriteCMD[2] = {0x01, 0x40};
  HAL_GPIO_WritePin(NCS_SRAM_SPI_GPIO_Port, NCS_SRAM_SPI_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(&hspi2, sramEnSequentialReadCMD, 2, 100);
  HAL_GPIO_WritePin(NCS_SRAM_SPI_GPIO_Port, NCS_SRAM_SPI_Pin, GPIO_PIN_SET);
  HAL_Delay(5);
  HAL_GPIO_WritePin(NCS_SRAM_SPI_GPIO_Port, NCS_SRAM_SPI_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(&hspi2, sramEnSequentialWriteCMD, 2, 100);
  HAL_GPIO_WritePin(NCS_SRAM_SPI_GPIO_Port, NCS_SRAM_SPI_Pin, GPIO_PIN_SET);

  // Variable to keep track of live display refresh
  uint32_t liveDisplayLastRefresh = HAL_GetTick();

  // Variable to prevent updating the LCD too often
  int updateLCD = 0;

  // UI variables
  int currentSlot = 0;
  int currentMenu = MENU_LIVE_DISPLAY;
  int editSlot = 0;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    if (encDir && !editSlot)
    {
      updateLCD = 1;
      // Cycle through menu options
      currentMenu += encDir;
      currentMenu = (currentMenu > 3) ? (0) : ((currentMenu < 0) ? (3) : currentMenu);

      switch (currentMenu)
      {
      case MENU_LIVE_DISPLAY:
        break;
      case MENU_WRITE_WORKOUT:
        snprintf(lcdStr1, 16, "Write Workout");
        snprintf(lcdStr2, 16, "B1 to set slot");
        break;
      case MENU_VIEW_HISTORY:
        snprintf(lcdStr1, 16, "Send to computer");
        snprintf(lcdStr2, 16, "B1 to set slot");
        break;
      case MENU_SETTINGS:
        snprintf(lcdStr1, 16, "Settings");
        snprintf(lcdStr2, 16, "B1 to enter");
        break;
      }
      encDir = 0;
    }

    // Handle each menu state
    switch (currentMenu)
    {
    case MENU_LIVE_DISPLAY:
      // Update live display
      if (HAL_GetTick() - liveDisplayLastRefresh >= 250)
      {
        liveDisplayLastRefresh = HAL_GetTick();
        updateLiveDisplay();
      }
      break;

    case MENU_WRITE_WORKOUT:
      if (encDir && editSlot)
      {
        currentSlot += encDir;
        currentSlot = (currentSlot > MAX_SAVE_SLOTS) ? (0) : ((currentSlot < 0) ? (MAX_SAVE_SLOTS) : currentSlot);

        snprintf(lcdStr1, 16, "Save to slot %d?", currentSlot);

        updateLCD = 1;
        encDir = 0;
      }

      if (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) && !editSlot)
      {
        editSlot = 1;
        snprintf(lcdStr1, 16, "Save to slot %d?", currentSlot);
        snprintf(lcdStr2, 16, "B1 to save", currentSlot);
        updateLCD = 1;
      }

      if (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) && editSlot)
      {
        WorkoutData_t workout;
        workout.magicNumber = WORKOUT_MAGIC;
        workout.totalSteps = totalSteps;
        workout.totalDistance = totalDistance;
        workout.totalCalories = totalCalories;
        workout.slotUsed = 1;

        uint16_t addr = WORKOUT_ADDR_BASE + (currentSlot * WORKOUT_SIZE);
        writeSRAM(addr, (uint8_t *)&workout, WORKOUT_SIZE);

        // Verify save
        WorkoutData_t verify;
        readSRAM(addr, (uint8_t *)&verify, WORKOUT_SIZE);

        LCD_ClearDisplay();
        LCD_Position(0, 0);
        if (verify.magicNumber == WORKOUT_MAGIC)
        {
          LCD_PrintString("Workout Saved!");
        }
        else
        {
          LCD_PrintString("Save Failed!");
        }

        // Wait for button release
        while (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin))
          ;

        HAL_Delay(1000);
        // Reset flags and return to live display
        currentMenu = MENU_LIVE_DISPLAY;
      }
      break;

    case MENU_VIEW_HISTORY:
      if (encDir && editSlot)
      {
        currentSlot += encDir;
        currentSlot = (currentSlot > MAX_SAVE_SLOTS) ? (0) : ((currentSlot < 0) ? (MAX_SAVE_SLOTS) : currentSlot);

        snprintf(lcdStr1, 16, "Send slot %d?", currentSlot);

        updateLCD = 1;
        encDir = 0;
      }

      if (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) && !editSlot)
      {
        editSlot = 1;
        snprintf(lcdStr1, 16, "Send slot %d?", currentSlot);
        snprintf(lcdStr2, 16, "B1 to send", currentSlot);
        updateLCD = 1;
      }

      if (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) && editSlot)
      {
        loadWorkout(currentSlot);

        LCD_ClearDisplay();
        LCD_PrintString("Sent!");

        // Wait for button release
        while (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin))
          ;

        HAL_Delay(1000);
        // Reset flags and return to live display
        currentMenu = MENU_LIVE_DISPLAY;
      }
      break;

    case MENU_SETTINGS:
      if (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin))
      {
        // Wait for button release
        while (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin))
          ;
        adjustSettings();
      }
      break;
    }

    if (updateLCD)
    {
      updateLCD = 0;
      lcdStr1[16] = 0;
      lcdStr2[16] = 0;

      // Update the LCD
      LCD_ClearDisplay();
      LCD_Position(0, 0);
      LCD_PrintString(lcdStr1);
      LCD_Position(1, 0);
      LCD_PrintString(lcdStr2);
    }

    // Process UART commands (keep this for remote access)
    uint8_t cmdBuffer;
    if (HAL_UART_Receive(&huart1, &cmdBuffer, 1, 10) == HAL_OK)
    {
      HAL_UART_Transmit(&huart1, &cmdBuffer, 1, 100);
      processUARTCommand(&cmdBuffer);
    }
  }
  /* USER CODE BEGIN 3 */
}
/* USER CODE END 3 */

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_I2C1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
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
  hi2c1.Init.Timing = 0x00201D2B;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
   */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
   */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */
}

/**
 * @brief SPI2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 7;
  hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */
}

/**
 * @brief TIM1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 18;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 63156;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);
}

/**
 * @brief TIM7 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM7_Init(void)
{

  /* USER CODE BEGIN TIM7_Init 0 */

  /* USER CODE END TIM7_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM7_Init 1 */

  /* USER CODE END TIM7_Init 1 */
  htim7.Instance = TIM7;
  htim7.Init.Prescaler = 23;
  htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim7.Init.Period = 65535;
  htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim7, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM7_Init 2 */

  /* USER CODE END TIM7_Init 2 */
}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 57600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */
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
  HAL_GPIO_WritePin(GPIOC, NCS_MEMS_SPI_Pin | EXT_RESET_Pin | GPIO_PIN_6 | LD6_Pin | LD4_Pin | LD5_Pin | STEPPER_B_Pin | STEPPER_NA_Pin | STEPPER_NB_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, NCS_SRAM_SPI_Pin | STEPPER_A_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : ENC_A_Pin ENC_B_Pin */
  GPIO_InitStruct.Pin = ENC_A_Pin | ENC_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : NCS_MEMS_SPI_Pin EXT_RESET_Pin PC6 LD6_Pin
                           LD4_Pin LD5_Pin STEPPER_B_Pin STEPPER_NA_Pin
                           STEPPER_NB_Pin */
  GPIO_InitStruct.Pin = NCS_MEMS_SPI_Pin | EXT_RESET_Pin | GPIO_PIN_6 | LD6_Pin | LD4_Pin | LD5_Pin | STEPPER_B_Pin | STEPPER_NA_Pin | STEPPER_NB_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : MEMS_INT1_Pin */
  GPIO_InitStruct.Pin = MEMS_INT1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(MEMS_INT1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PA2 PA3 PA6 PA7 */
  GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_6 | GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF3_TSC;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 */
  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF3_TSC;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : ESTOP_Pin */
  GPIO_InitStruct.Pin = ESTOP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(ESTOP_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : I2C2_SCL_Pin I2C2_SDA_Pin */
  GPIO_InitStruct.Pin = I2C2_SCL_Pin | I2C2_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF1_I2C2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : NCS_SRAM_SPI_Pin STEPPER_A_Pin */
  GPIO_InitStruct.Pin = NCS_SRAM_SPI_Pin | STEPPER_A_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI2_3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_3_IRQn);

  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/// @brief
/// @param motorSteps
/// @return
int distanceTraveled(int motorSteps)
{
  return (0.1 * M_PI) * motorSteps / STEPS_PER_REV;
}

/// @brief
/// @param distanceTraveled
/// @return
int stepsWalked(int distanceTraveled)
{
  return distanceTraveled * 2;
}

int caloriesBurned(int stepsWalked)
{
  return stepsWalked * 0.04;
}

/// @brief Read bytes sequentially from the SRAM over SPI starting from addr and ending at addr + size
/// @param addr
/// @param rd_array
/// @param size
void readSRAM(uint16_t addr, uint8_t *rd_array, uint16_t size)
{
  uint8_t spi_rd_buf[size + 3];
  uint8_t spi_wr_buf[size + 3];

  spi_wr_buf[0] = SRAM_RD_CMD;
  spi_wr_buf[1] = (addr >> 8) & 0xFF;
  spi_wr_buf[2] = addr & 0xFF;
  for (int i = 0; i < size; i++)
  {
    spi_wr_buf[i + 3] = 0xAA; // Dummy value to be able to read
  }

  HAL_GPIO_WritePin(NCS_SRAM_SPI_GPIO_Port, NCS_SRAM_SPI_Pin, GPIO_PIN_RESET);
  HAL_SPI_TransmitReceive(&hspi2, spi_wr_buf, spi_rd_buf, size + 3, 100);
  HAL_GPIO_WritePin(NCS_SRAM_SPI_GPIO_Port, NCS_SRAM_SPI_Pin, GPIO_PIN_SET);
  HAL_Delay(2);

  for (int i = 0; i < size; i++)
  {
    rd_array[i] = spi_rd_buf[i + 3]; // Copy bytes from the read buffer into the destination array
  }
}

/// @brief Write bytes sequentially to the SRAM over SPI starting from addr and ending at addr + size
/// @param addr
/// @param wr_array
/// @param size
void writeSRAM(uint16_t addr, uint8_t *wr_array, uint16_t size)
{
  uint8_t spi_wr_buf[size + 3];

  spi_wr_buf[0] = SRAM_WR_CMD;
  spi_wr_buf[1] = (addr >> 8) & 0xFF;
  spi_wr_buf[2] = addr & 0xFF;
  for (int i = 0; i < size; i++)
  {
    spi_wr_buf[i + 3] = wr_array[i]; // Copy bytes from the write array into the write buffer
  }

  HAL_GPIO_WritePin(NCS_SRAM_SPI_GPIO_Port, NCS_SRAM_SPI_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(&hspi2, spi_wr_buf, size + 3, 100);
  HAL_GPIO_WritePin(NCS_SRAM_SPI_GPIO_Port, NCS_SRAM_SPI_Pin, GPIO_PIN_SET);
  HAL_Delay(2);
}

/// @brief Set the servo position to servoPosDeg by changing the PWM pulse width.
///
/// The valid range for the servo position is about 0-210 deg
/// @param servoPosDeg
void setServoPos(double servoPosDeg)
{
  // Constrain servoPosDeg to between 0 and 210
  servoPosDeg = (servoPosDeg > 210) ? (210) : ((servoPosDeg < 0) ? (0) : (servoPosDeg));

  // Map the servo position in degrees to a valid TIM1->CCR2 value
  // The servo only responds to pulse widths between about 350-2600 us
  uint16_t regVal = (((servoPosDeg * (2600 - 350) / 210) + 350) / 25000) * 63156;
  TIM1->CCR2 = regVal;
}

/// @brief Set the stepper motor's speed to rpm and set the rotation direction
/// @param rpm
/// @param direction +-1
void setStepperSpeed(double rpm, int direction)
{
  if (rpm == 0)
  {
    HAL_TIM_Base_Stop_IT(&htim7);
  }
  else
  {
    HAL_TIM_Base_Start_IT(&htim7);
  }
  TIM7->ARR = 60.0 * 48000000.0 / 24 / rpm / (double)STEPS_PER_REV - 1;
  stepDir = direction;
}

void updateLiveDisplay(void)
{
  char lcdStr1[17];
  char lcdStr2[17];

  // Calculate distance traveled
  totalDistance = distanceTraveled(step); // meters

  // Calculate steps walked
  totalSteps = stepsWalked(totalDistance); // average step length meters

  // Calculate calories burned
  totalCalories = caloriesBurned(totalSteps); // about 0.04 calories per step

  // Update LCD
  snprintf(lcdStr1, 17, "Dist: %d m", totalDistance);
  snprintf(lcdStr2, 17, "Cal: %d", totalCalories);

  lcdStr1[16] = 0;
  lcdStr1[16] = 0;

  LCD_ClearDisplay();
  LCD_Position(0, 0);
  LCD_PrintString(lcdStr1);
  LCD_Position(1, 0);
  LCD_PrintString(lcdStr2);
}

void adjustSettings(void)
{
  char lcdStr1[17];
  char lcdStr2[17];
  int updateLCD = 0;

  int exitflag = 0;
  int menuIndex = 0;
  int settingMode = 0;

  while (!exitflag)
  {
    if (encDir)
    {
      updateLCD = 1;

      if (settingMode) // Adjust current setting if in settingMode
      {
        switch (menuIndex)
        {
        case 0:
          targetStepperRPM += encDir * 2; // Adjust RPM by 2 per encoder tick
          targetStepperRPM = (targetStepperRPM > 21) ? (21) : ((targetStepperRPM < 0) ? (0) : (targetStepperRPM));
          break;
        case 1:
          targetServoPosDeg += encDir * 5; // Adjust servo position by 5 degrees per encoder tick
          targetServoPosDeg = (targetServoPosDeg > 30) ? (30) : ((targetServoPosDeg < 0) ? (0) : (targetServoPosDeg));
          break;
        case 2:
          exitflag = 1;
          break;
        }
      }
      else if (!settingMode) // Adjust menu index if not in settingMode
      {
        menuIndex += encDir;
        menuIndex = (menuIndex > 2) ? (0) : ((menuIndex < 0) ? (2) : (menuIndex));
      }

      encDir = 0;
    }

    // Check B1 button press to enter/exit adjustment mode
    if (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin))
    {
      updateLCD = 1;
      if (settingMode == 0)
      {
        settingMode = 1; // Enter adjustment mode
      }
      else
      {
        // Exiting adjustment mode — apply the settings
        setStepperSpeed(targetStepperRPM, 1);
        setServoPos(targetServoPosDeg);
        settingMode = 0; // Exit adjustment mode
      }
      while (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin))
        ; // Wait for button release
    }

    // Update display with current menu
    if (updateLCD)
    {
      updateLCD = 0;
      switch (menuIndex)
      {
      case 0:
        snprintf(lcdStr1, sizeof(lcdStr1), "Set Speed:");
        break;
      case 1:
        snprintf(lcdStr1, sizeof(lcdStr1), "Set Incline:");
        break;
      case 2:
        snprintf(lcdStr1, sizeof(lcdStr1), "Exit Settings:");
        break;
      }

      if (settingMode)
      {
        switch (menuIndex)
        {
        case 0:
          snprintf(lcdStr2, sizeof(lcdStr2), "%d RPM", targetStepperRPM);
          break;
        case 1:
          snprintf(lcdStr2, sizeof(lcdStr2), "%d deg", targetServoPosDeg);
          break;
        case 2:
          snprintf(lcdStr2, sizeof(lcdStr2), "Move enc to Exit");
          break;
        }
      }
      else
      {
        snprintf(lcdStr2, sizeof(lcdStr2), "Press B1 to Edit");
      }

      lcdStr1[16] = 0;
      lcdStr2[16] = 0;

      // Update the LCD
      LCD_ClearDisplay();
      LCD_Position(0, 0);
      LCD_PrintString(lcdStr1);
      LCD_Position(1, 0);
      LCD_PrintString(lcdStr2);
    }
  }
}

void saveWorkout(uint8_t slot)
{
  // Add debug output before saving
  char debugMsg[64];
  snprintf(debugMsg, sizeof(debugMsg),
           "\r\nSaving to slot %d: Steps=%d, Dist=%d, Cal=%d\r\n",
           slot, totalSteps, totalDistance, totalCalories);
  HAL_UART_Transmit(&huart1, (uint8_t *)debugMsg, strlen(debugMsg), 100);

  WorkoutData_t workout;
  workout.magicNumber = WORKOUT_MAGIC;
  workout.totalSteps = totalSteps;
  workout.totalDistance = totalDistance;
  workout.totalCalories = totalCalories;
  workout.slotUsed = 1;

  uint16_t addr = WORKOUT_ADDR_BASE + (slot * WORKOUT_SIZE);

  // Write the structure to SRAM
  writeSRAM(addr, (uint8_t *)&workout, WORKOUT_SIZE);

  // Verify the write by reading back
  WorkoutData_t verify;
  readSRAM(addr, (uint8_t *)&verify, WORKOUT_SIZE);

  if (verify.magicNumber == WORKOUT_MAGIC)
  {
    char uartBuffer[64];
    snprintf(uartBuffer, sizeof(uartBuffer),
             "Successfully saved workout in slot %d:\r\nSteps: %ld\r\nDist: %ld m\r\nCal: %ld\r\n",
             slot, verify.totalSteps, verify.totalDistance, verify.totalCalories);
    HAL_UART_Transmit(&huart1, (uint8_t *)uartBuffer, strlen(uartBuffer), 100);
  }
}

void loadWorkout(uint8_t slot)
{
  WorkoutData_t workout;
  uint16_t addr = WORKOUT_ADDR_BASE + (slot * WORKOUT_SIZE);

  // Clear the structure before reading
  memset(&workout, 0, WORKOUT_SIZE);

  // Read from SRAM
  readSRAM(addr, (uint8_t *)&workout, WORKOUT_SIZE);

  // Debug output of raw values
  char debugMsg[64];
  snprintf(debugMsg, sizeof(debugMsg),
           "\r\nRaw data from slot %d: Magic=0x%04X, Used=%d\r\n",
           slot, workout.magicNumber, workout.slotUsed);
  HAL_UART_Transmit(&huart1, (uint8_t *)debugMsg, strlen(debugMsg), 100);

  if (workout.magicNumber == WORKOUT_MAGIC && workout.slotUsed)
  {
    char uartBuffer[64];
    snprintf(uartBuffer, sizeof(uartBuffer),
             "Slot %d workout:\r\nSteps: %ld\r\nDistance: %ld m\r\nCalories: %ld\r\n",
             slot, workout.totalSteps, workout.totalDistance, workout.totalCalories);
    HAL_UART_Transmit(&huart1, (uint8_t *)uartBuffer, strlen(uartBuffer), 100);
  }
  else
  {
    char uartBuffer[32];
    snprintf(uartBuffer, sizeof(uartBuffer),
             "No valid workout in slot %d\r\n", slot);
    HAL_UART_Transmit(&huart1, (uint8_t *)uartBuffer, strlen(uartBuffer), 100);
  }
}

void displayWorkoutHistory(void)
{
  char uartBuffer[32];
  snprintf(uartBuffer, sizeof(uartBuffer), "\r\nWorkout History:\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t *)uartBuffer, strlen(uartBuffer), 100);

  for (uint8_t slot = 0; slot < MAX_SAVE_SLOTS; slot++)
  {
    loadWorkout(slot);
  }
}

void processUARTCommand(uint8_t *cmdBuffer)
{
  switch (cmdBuffer[0])
  {
  case 'v':
  case 'V':
    // View all workout slots
    handleViewWorkouts();
    break;

  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    // View specific slot
    loadWorkout(cmdBuffer[0] - '0');
    break;

  case 'h':
  case 'H':
    // Help menu
    {
      const char *helpMsg =
          "\r\nTreadmill Workout Commands:\r\n"
          "v - View all workout slots\r\n"
          "0-9 - View specific slot\r\n"
          "h - Show this help menu\r\n";
      HAL_UART_Transmit(&huart1, (uint8_t *)helpMsg, strlen(helpMsg), 100);
    }
    break;
  }
}

void handleViewWorkouts(void)
{
  char uartBuffer[128];
  snprintf(uartBuffer, sizeof(uartBuffer), "\r\nStored Workouts:\r\n");
  HAL_UART_Transmit(&huart1, (uint8_t *)uartBuffer, strlen(uartBuffer), 100);

  for (uint8_t slot = 0; slot < MAX_SAVE_SLOTS; slot++)
  {
    WorkoutData_t workout;
    uint16_t addr = WORKOUT_ADDR_BASE + (slot * WORKOUT_SIZE);

    readSRAM(addr, (uint8_t *)&workout, WORKOUT_SIZE);

    if (workout.magicNumber == WORKOUT_MAGIC && workout.slotUsed)
    {
      snprintf(uartBuffer, sizeof(uartBuffer),
               "\r\nSlot %d:\r\nSteps: %ld\r\nDistance: %ld m\r\nCalories: %ld\r\n",
               slot, workout.totalSteps, workout.totalDistance, workout.totalCalories);
    }
    else
    {
      snprintf(uartBuffer, sizeof(uartBuffer),
               "\r\nSlot %d: [empty]\r\n", slot);
    }
    HAL_UART_Transmit(&huart1, (uint8_t *)uartBuffer, strlen(uartBuffer), 100);
  }
}
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
