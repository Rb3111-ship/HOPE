/*
 * time_service.c
 *
 * Created on: 28 Apr 2026
 * Author: whp27
 */

#include "time_service.h"
#include "DS3231_RTC_driver.h"
#include <stdint.h>
#include <stdbool.h>
#include "tasks.h"

/* Global buffers tracking un-marshaled time packet parameters */
static uint8_t time_data[3] = { 0 };
static uint8_t hour_data = 0;
static uint8_t mins_data = 0;
static bool time_valid = false; // true once the RTC has been read successfully

/**
 * @brief Pulls raw data from the external RTC and maps components onto local structures.
 */
static void split_time(void) {
	uint8_t *buff = get_RTC_Data();
	if (buff == NULL) {
		return; // read failed: keep showing the last good time
	}
	for (int i = 1; i < 3; i++) {

		if (i == 1)
			mins_data = buff[i];
		else
			hour_data = buff[i];
	}
	time_valid = true;
}

/**
 * @brief True once a real time has been read from (or written to) the RTC.
 * @details Until then get_Time() reports 00:00, which must not be used to
 *          decide whether alarms have already fired today.
 */
bool time_is_valid(void) {
	return time_valid;
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

	// Gate physical reads to a 1-second operational period (but read straight
	// away until the first good read, so boot doesn't show 00:00 for a second)
	if (!time_valid || (now - last_sensor_read) >= pdMS_TO_TICKS(1000)) {
		last_sensor_read = now;

		split_time();
	}
	*hours = hour_data;
	*mins = mins_data;
}

/**
 * @brief Forces a baseline zeroing on seconds parameters before committing to storage.
 */
static void update_time(void) {
	time_data[0] = 0;
	if (set_RTC_Data(time_data)) {
		// Show the new time straight away instead of waiting for the next 1 s read
		mins_data = time_data[1];
		hour_data = time_data[2];
		time_valid = true;
	}
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
void confirm_time(void) {
	update_time();   // write both hours and mins together
}
