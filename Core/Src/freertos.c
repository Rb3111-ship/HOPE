/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/*
 * Called by the kernel when a task overruns its stack (configCHECK_FOR_STACK_OVERFLOW = 2).
 * Debug builds stop here so the debugger shows which task (pcTaskName);
 * release builds reset the device so it recovers on its own.
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
	(void) xTask;
	(void) pcTaskName;
	taskDISABLE_INTERRUPTS();
#ifdef DEBUG
	for (;;) {
	}
#else
	NVIC_SystemReset();
#endif
}

/*
 * Called when pvPortMalloc() fails (configUSE_MALLOC_FAILED_HOOK = 1), i.e. the
 * FreeRTOS heap (configTOTAL_HEAP_SIZE) is exhausted.
 */
void vApplicationMallocFailedHook(void) {
	taskDISABLE_INTERRUPTS();
#ifdef DEBUG
	for (;;) {
	}
#else
	NVIC_SystemReset();
#endif
}


/* USER CODE END Application */

