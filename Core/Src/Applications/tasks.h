/*
 * ui_task.h
 *
 *  Created on: 21 Apr 2026
 *      Author: whp27
 */

#ifndef SRC_APPLICATIONS_TASKS_H_
#define SRC_APPLICATIONS_TASKS_H_

#include "FreeRTOS.h"
#include "task.h"

// task function declarations
void ui_Task(void * pvParameters);
void music_Task(void * pvParameters);
void light_Task(void * pvParameters);


#endif /* SRC_APPLICATIONS_TASKS_H_ */
