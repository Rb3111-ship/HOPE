/*
 * queueCreator.h
 *
 *  Created on: 21 Apr 2026
 *      Author: whp27
 */

#ifndef SRC_APPLICATIONS_APP_QUEUE_H_
#define SRC_APPLICATIONS_APP_QUEUE_H_

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

extern QueueHandle_t uiQueueHandle;
extern QueueHandle_t musicQueueHandle;
extern QueueHandle_t lightQueueHandle;


#endif /* SRC_APPLICATIONS_APP_QUEUE_H_ */
