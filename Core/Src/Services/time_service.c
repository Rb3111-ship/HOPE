/*
 * time_service.c
 *
 *  Created on: 28 Apr 2026
 *      Author: whp27
 */

#include "time_service.h"
#include "DS3231_RTC_driver.h"
#include <stdint.h>

uint8_t time_data [3]= {0};
uint8_t hour_data = 0;
uint8_t mins_data = 0;

void spit_time() {
	uint8_t *buff = get_RTC_Data();
	for (int i = 1; i < 3; i++) {

		if (i == 1)
			mins_data = buff[i];
		else
			hour_data = buff[i];
	}

}

void get_Time(uint8_t *hours, uint8_t *mins) {
    spit_time();
    *hours = hour_data;
    *mins  = mins_data;
}


void update_time(){
	time_data[0] = 0;
	set_RTC_Data(time_data);
}

void set_TimeMins(uint8_t mins) {
	time_data[1] = mins;
}

void set_TimeH(uint8_t hours) {
	time_data[2] = hours;
	update_time();
}

void confirm_time() {
    update_time();   // write both hours and mins together
}

