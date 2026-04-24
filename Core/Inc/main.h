/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_TOGGLE_Pin GPIO_PIN_2
#define LED_TOGGLE_GPIO_Port GPIOB
#define BTN_MUSIC_Pin GPIO_PIN_12
#define BTN_MUSIC_GPIO_Port GPIOB
#define BTN_MUSIC_EXTI_IRQn EXTI15_10_IRQn
#define BTN_TIMER_Pin GPIO_PIN_13
#define BTN_TIMER_GPIO_Port GPIOB
#define BTN_TIMER_EXTI_IRQn EXTI15_10_IRQn
#define BTN_VOL_UP_Pin GPIO_PIN_14
#define BTN_VOL_UP_GPIO_Port GPIOB
#define BTN_VOL_UP_EXTI_IRQn EXTI15_10_IRQn
#define BTN_DWN_Pin GPIO_PIN_15
#define BTN_DWN_GPIO_Port GPIOB
#define BTN_DWN_EXTI_IRQn EXTI15_10_IRQn
#define BTN_LIGHT_Pin GPIO_PIN_2
#define BTN_LIGHT_GPIO_Port GPIOD
#define BTN_LIGHT_EXTI_IRQn EXTI2_IRQn
#define BTN_UP_Pin GPIO_PIN_3
#define BTN_UP_GPIO_Port GPIOB
#define BTN_UP_EXTI_IRQn EXTI3_IRQn
#define BTN_PLAY_Pin GPIO_PIN_4
#define BTN_PLAY_GPIO_Port GPIOB
#define BTN_PLAY_EXTI_IRQn EXTI4_IRQn
#define BTN_VOL_DWN_Pin GPIO_PIN_5
#define BTN_VOL_DWN_GPIO_Port GPIOB
#define BTN_VOL_DWN_EXTI_IRQn EXTI9_5_IRQn
#define DHT22_DATA_Pin GPIO_PIN_8
#define DHT22_DATA_GPIO_Port GPIOB
#define BLE_SWITCH_Pin GPIO_PIN_9
#define BLE_SWITCH_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
