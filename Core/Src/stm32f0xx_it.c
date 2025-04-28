/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    stm32f0xx_it.c
 * @brief   Interrupt Service Routines.
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
#include "stm32f0xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "I2C_LCD_PCF8574.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

extern TIM_HandleTypeDef htim1;
extern volatile int encDir;
extern volatile int step;
extern int stepDir;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern TIM_HandleTypeDef htim7;
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M0 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
  while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVC_IRQn 0 */

  /* USER CODE END SVC_IRQn 0 */
  /* USER CODE BEGIN SVC_IRQn 1 */

  /* USER CODE END SVC_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F0xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f0xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles EXTI line 2 and 3 interrupts.
  */
void EXTI2_3_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI2_3_IRQn 0 */
  // STOP all motors and display LCD message to reset the microcontroller to restart

  char stopMsg1[17] = "E-STOP activated";
  stopMsg1[16] = 0;
  char stopMsg2[17] = "Restart to clear";
  stopMsg2[16] = 0;

  // Stop servo
  HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);

  // Stop stepper
  HAL_TIM_Base_Stop_IT(&htim7);
  HAL_GPIO_WritePin(STEPPER_A_GPIO_Port, STEPPER_A_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(STEPPER_B_GPIO_Port, STEPPER_B_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(STEPPER_NA_GPIO_Port, STEPPER_NA_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(STEPPER_NB_GPIO_Port, STEPPER_NB_Pin, GPIO_PIN_RESET);

  // Display user message to restart to clear the E-STOP
  LCD_ClearDisplay();
  LCD_Position(0, 0);
  LCD_PrintString(stopMsg1);
  LCD_Position(1, 0);
  LCD_PrintString(stopMsg2);

  while (1)
  {
    // Wait forever until the microcontroller restarts
  }

  /* USER CODE END EXTI2_3_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(ESTOP_Pin);
  /* USER CODE BEGIN EXTI2_3_IRQn 1 */

  /* USER CODE END EXTI2_3_IRQn 1 */
}

/**
  * @brief This function handles EXTI line 4 to 15 interrupts.
  */
void EXTI4_15_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI4_15_IRQn 0 */
  if (HAL_GPIO_ReadPin(ENC_A_GPIO_Port, ENC_A_Pin) && !HAL_GPIO_ReadPin(ENC_B_GPIO_Port, ENC_B_Pin))
  {
    encDir = 1;
  }
  else if (HAL_GPIO_ReadPin(ENC_B_GPIO_Port, ENC_B_Pin) && !HAL_GPIO_ReadPin(ENC_A_GPIO_Port, ENC_A_Pin))
  {
    encDir = -1;
  }
  /* USER CODE END EXTI4_15_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(ENC_A_Pin);
  HAL_GPIO_EXTI_IRQHandler(ENC_B_Pin);
  /* USER CODE BEGIN EXTI4_15_IRQn 1 */

  /* USER CODE END EXTI4_15_IRQn 1 */
}

/**
  * @brief This function handles TIM7 global interrupt.
  */
void TIM7_IRQHandler(void)
{
  /* USER CODE BEGIN TIM7_IRQn 0 */
  // Full step drive pattern
  step += stepDir;

  // To extract the proper sequence, we only need to look at the last 2 bits of the number
  // This is obvious for positive numbers, but, for negative numbers, it works only if the number is in 2's complement
  // This also only works for sequences that have a length that is a power of 2
  switch (step & 3)
  {
  case 0:
    HAL_GPIO_WritePin(STEPPER_A_GPIO_Port, STEPPER_A_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(STEPPER_B_GPIO_Port, STEPPER_B_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(STEPPER_NA_GPIO_Port, STEPPER_NA_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEPPER_NB_GPIO_Port, STEPPER_NB_Pin, GPIO_PIN_RESET);
    break;
  case 1:
    HAL_GPIO_WritePin(STEPPER_A_GPIO_Port, STEPPER_A_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEPPER_B_GPIO_Port, STEPPER_B_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(STEPPER_NA_GPIO_Port, STEPPER_NA_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(STEPPER_NB_GPIO_Port, STEPPER_NB_Pin, GPIO_PIN_RESET);
    break;
  case 2:
    HAL_GPIO_WritePin(STEPPER_A_GPIO_Port, STEPPER_A_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEPPER_B_GPIO_Port, STEPPER_B_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEPPER_NA_GPIO_Port, STEPPER_NA_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(STEPPER_NB_GPIO_Port, STEPPER_NB_Pin, GPIO_PIN_SET);
    break;
  case 3:
    HAL_GPIO_WritePin(STEPPER_A_GPIO_Port, STEPPER_A_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(STEPPER_B_GPIO_Port, STEPPER_B_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEPPER_NA_GPIO_Port, STEPPER_NA_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEPPER_NB_GPIO_Port, STEPPER_NB_Pin, GPIO_PIN_SET);
    break;
  default:
    break;
  }

  /* USER CODE END TIM7_IRQn 0 */
  HAL_TIM_IRQHandler(&htim7);
  /* USER CODE BEGIN TIM7_IRQn 1 */

  /* USER CODE END TIM7_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
