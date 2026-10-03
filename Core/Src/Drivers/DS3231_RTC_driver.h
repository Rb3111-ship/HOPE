/*
 * DS3231_RTC_driver.h
 *
 * Created on: 28 Apr 2026
 * Author: whp27
 */

#ifndef SRC_DRIVERS_DS3231_RTC_DRIVER_H_
#define SRC_DRIVERS_DS3231_RTC_DRIVER_H_

#include <stdint.h>
#include <stdbool.h>

// Low-Level Hardware Driver Application Programming Interface Prototypes
uint8_t * get_RTC_Data(void);             // NULL if the read failed
bool set_RTC_Data(uint8_t *deci_Time); // false if the write failed

#endif /* SRC_DRIVERS_DS3231_RTC_DRIVER_H_ */
