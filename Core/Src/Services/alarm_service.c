/*
 * alarm_service.c
 *
 * Created on: 5 May 2026
 * Author: whp27
 */

#include "alarm_service.h"
#include "ui_renderer.h" // Include this to access UI_SetAlarmListContext
#include <string.h>
#include "time_service.h"
#include "AT24C32_driver.h"
#include <stdbool.h>

#define MAX_ALARMS 10

/* EEPROM record (one 32-byte page at address 0):
 *   [0]       magic/version byte
 *   [1..30]   10 x { is_active, hour, minute }
 *   [31]      checksum = ~(sum of bytes 0..30)
 * A new or erased chip (all 0xFF) fails the magic/checksum test and is ignored. */
#define ALARM_EEPROM_ADDR   0x0000U
#define ALARM_EEPROM_MAGIC  0xA7U
#define ALARM_RECORD_LEN    (2U + (MAX_ALARMS * 3U))
_Static_assert(ALARM_RECORD_LEN <= AT24C32_PAGE_SIZE, "alarm record must fit one EEPROM page");

// Core array allocating physical block properties for up to 10 localised software alarms
SoftwareAlarm my_backend_alarms_array[MAX_ALARMS];

static bool alarms_synced = false;  // triggered_today flags computed from a real RTC time
static uint32_t last_now = 0;       // previous check_alarm() time, to spot midnight

/**
 * @brief Zero-initializes storage sectors and pushes context bindings out to display systems.
 */
void alarm_service_init(void) {
	// 1. Initialise your array (set all to inactive/0)
	memset(my_backend_alarms_array, 0, sizeof(my_backend_alarms_array));

	// 2. Pass the pointer to the UI so it knows what to draw
	UI_SetAlarmListContext(my_backend_alarms_array, MAX_ALARMS);
}

/**
 * @brief Mathematical scaling utility mapping standard 24-hour markers onto a running scalar tally.
 */
static uint32_t seconds_since_midnight(uint8_t h, uint8_t m) {
	return ((h * 3600) + (m * 60));
}

/**
 * @brief Extracts the immediate timeline tracking properties as a raw numerical total.
 */
static uint32_t current_time(void) {
	uint8_t hours, mins;
	get_Time(&hours, &mins);

	return seconds_since_midnight(hours, mins);
}

/**
 * @brief Marks alarms whose time has already passed today as done, so they don't
 *        fire the moment the clock becomes known (boot) or is changed by the user.
 */
static void alarm_service_resync(void) {
	uint32_t now = current_time();
	for (int i = 0; i < MAX_ALARMS; i++) {
		my_backend_alarms_array[i].triggered_today =
				(my_backend_alarms_array[i].alarm_time <= now) ? 1 : 0;
	}
	last_now = now;
	alarms_synced = true;
}

/**
 * @brief Call after the user sets the clock.
 */
void alarm_service_time_changed(void) {
	if (time_is_valid()) {
		alarm_service_resync();
	} else {
		alarms_synced = false; // resync on the first good RTC read
	}
}

/**
 * @brief Writes all alarms to the RTC module's EEPROM so they survive power loss.
 */
static void alarm_service_save(void) {
	uint8_t rec[ALARM_RECORD_LEN];
	uint8_t sum = 0;

	rec[0] = ALARM_EEPROM_MAGIC;
	for (int i = 0; i < MAX_ALARMS; i++) {
		rec[1 + i * 3] = my_backend_alarms_array[i].is_active;
		rec[2 + i * 3] = my_backend_alarms_array[i].hour;
		rec[3 + i * 3] = my_backend_alarms_array[i].minute;
	}
	for (unsigned i = 0; i < ALARM_RECORD_LEN - 1; i++) {
		sum += rec[i];
	}
	rec[ALARM_RECORD_LEN - 1] = (uint8_t) ~sum;

	eeprom_write_page(ALARM_EEPROM_ADDR, rec, ALARM_RECORD_LEN);
}

/**
 * @brief Restores alarms saved in the EEPROM. Call once from a task (uses the I2C mutex).
 * @details Leaves all alarms empty if the EEPROM is blank, missing or corrupted.
 */
void alarm_service_load(void) {
	uint8_t rec[ALARM_RECORD_LEN];
	uint8_t sum = 0;

	if (!eeprom_read(ALARM_EEPROM_ADDR, rec, ALARM_RECORD_LEN)) {
		return;
	}
	for (unsigned i = 0; i < ALARM_RECORD_LEN - 1; i++) {
		sum += rec[i];
	}
	if (rec[0] != ALARM_EEPROM_MAGIC || rec[ALARM_RECORD_LEN - 1] != (uint8_t) ~sum) {
		return;
	}

	for (int i = 0; i < MAX_ALARMS; i++) {
		uint8_t active = rec[1 + i * 3];
		uint8_t h = rec[2 + i * 3];
		uint8_t m = rec[3 + i * 3];
		if (active == 1 && h < 24 && m < 60) {
			my_backend_alarms_array[i].is_active = 1;
			my_backend_alarms_array[i].hour = h;
			my_backend_alarms_array[i].minute = m;
			my_backend_alarms_array[i].alarm_time = seconds_since_midnight(h, m);
		}
	}
	alarms_synced = false; // decide which already fired today once the time is known
}

/**
 * @brief Target callback routine formatting parameters when modifications commit from user interfaces.
 * @details Evaluates immediate timeline relations to catch and adjust latch states for events added post-trigger time.
 */
void UI_OnAlarmAdded_Callback(uint8_t list_index, uint8_t alarm_edit_h,
		uint8_t alarm_edit_m) {
	if (list_index >= MAX_ALARMS)
		return;
	my_backend_alarms_array[list_index].hour = alarm_edit_h;
	my_backend_alarms_array[list_index].minute = alarm_edit_m;
	uint32_t alarm_time = seconds_since_midnight(alarm_edit_h, alarm_edit_m);

	// Evaluate context constraints to prevent unintended firing loops on late additions
	if (current_time() >= alarm_time) {
		my_backend_alarms_array[list_index].triggered_today = 1; //triggered today
	} else
		my_backend_alarms_array[list_index].triggered_today = 0; //not triggered

	my_backend_alarms_array[list_index].alarm_time = alarm_time;
	my_backend_alarms_array[list_index].is_active = 1; // Turn it on
	alarm_service_save();
}

void UI_OnAlarmDeleted_Callback(uint8_t list_index) {
	if (list_index >= MAX_ALARMS)
		return;
	my_backend_alarms_array[list_index].is_active = 0; // Soft delete
	alarm_service_save();
}

/**
 * @brief Scanning evaluation engine iterating across all enabled software alarm blocks.
 * @details Assesses current time records against targets, updates trigger flags, and catches day transitions to roll flags back down.
 * @return True if an un-triggered active alarm threshold has been broken, False otherwise.
 */
bool check_alarm(void) {
	// Never judge alarms against the 00:00 placeholder before the RTC is read
	if (!time_is_valid()) {
		return false;
	}
	if (!alarms_synced) {
		alarm_service_resync();
		return false;
	}

	uint32_t now = current_time();

	// Time went backwards = a new day started (or the clock was moved back):
	// every alarm is due again. Works for an alarm at 00:00 too.
	if (now < last_now) {
		for (int i = 0; i < MAX_ALARMS; i++) {
			my_backend_alarms_array[i].triggered_today = 0;
		}
	}
	last_now = now;

	for (int i = 0; i < MAX_ALARMS; i++) {
		if (my_backend_alarms_array[i].is_active
				&& (my_backend_alarms_array[i].alarm_time <= now)
				&& !(my_backend_alarms_array[i].triggered_today)) {
			my_backend_alarms_array[i].triggered_today = 1; //triggered
			return true;
		}
	}
	return false;
}
