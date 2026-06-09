/*
 * time_service.c
 *
 * Created on: 28 Apr 2026
 * Author: whp27
 */

#include "time_service.h"
#include "DS3231_RTC_driver.h"
#include <stdint.h>
#include "tasks.h"

/* Global buffers tracking un-marshaled time packet parameters */
uint8_t time_data[3] = { 0 };
static uint8_t hour_data = 0;
static uint8_t mins_data = 0;

/**
 * @brief Pulls raw data from the external RTC and maps components onto local structures.
 */
void split_time() {
	uint8_t *buff = get_RTC_Data();
	for (int i = 1; i < 3; i++) {

		if (i == 1)
			mins_data = buff[i];
		else
			hour_data = buff[i];
	}
}

/**
 * @brief Exposes the current validated hour and minute properties back to the system.
 * @details Throttles physical driver reads via a 1000ms scheduler gate to limit redundant I2C bus traffic.
 * @param hours Reference location where the current hour metric will be committed.
 * @param mins Reference location where the current minute metric will be committed.
 */
void get_Time(uint8_t *hours, uint8_t *mins) {
	static uint32_t last_sensor_read = 0;

	uint32_t now = xTaskGetTickCount();

	// Gate physical reads to a 1-second operational period
	if ((now - last_sensor_read) >= pdMS_TO_TICKS(1000)) {
		last_sensor_read = now;

		split_time();
	}
	*hours = hour_data;
	*mins = mins_data;
}

/**
 * @brief Forces a baseline zeroing on seconds parameters before committing to storage.
 */
void update_time() {
	time_data[0] = 0;
	set_RTC_Data(time_data);
}

void set_TimeMins(uint8_t mins) {
	time_data[1] = mins;
}

void set_TimeH(uint8_t hours) {
	time_data[2] = hours;
//	update_time();
}

/**
 * @brief Flushes the locally modified staging array back up into the physical hardware registers.
 */
void confirm_time() {
	update_time();   // write both hours and mins together
}
