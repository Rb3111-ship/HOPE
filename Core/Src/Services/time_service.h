/*
 * time_service.h
 *
 *  Created on: 28 Apr 2026
 *      Author: whp27
 */

#ifndef SRC_SERVICES_TIME_SERVICE_H_
#define SRC_SERVICES_TIME_SERVICE_H_

#include "DS3231_RTC_driver.h"
#include <stdint.h>

void get_Time(uint8_t *hours, uint8_t *mins);

void set_TimeMins(uint8_t  mins);
void set_TimeH(uint8_t  hours);
void confirm_time();


#endif /* SRC_SERVICES_TIME_SERVICE_H_ */
