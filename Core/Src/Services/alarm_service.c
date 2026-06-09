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
#include <stdbool.h>

// Core array allocating physical block properties for up to 10 localised software alarms
SoftwareAlarm my_backend_alarms_array[10];

/**
 * @brief Zero-initializes storage sectors and pushes context bindings out to display systems.
 */
void alarm_service_init(void) {
	// 1. Initialise your array (set all to inactive/0)
	memset(my_backend_alarms_array, 0, sizeof(my_backend_alarms_array));

	// 2. Pass the pointer to the UI so it knows what to draw
	UI_SetAlarmListContext(my_backend_alarms_array, 10);
}

/**
 * @brief Mathematical scaling utility mapping standard 24-hour markers onto a running scalar tally.
 */
uint32_t seconds_since_midnight(uint8_t h, uint8_t m) {
	return ((h * 3600) + (m * 60));
}

/**
 * @brief Extracts the immediate timeline tracking properties as a raw numerical total.
 */
uint32_t current_time() {
	uint8_t hours, mins;
	get_Time(&hours, &mins);

	return seconds_since_midnight(hours, mins);
}

/**
 * @brief Target callback routine formatting parameters when modifications commit from user interfaces.
 * @details Evaluates immediate timeline relations to catch and adjust latch states for events added post-trigger time.
 */
void UI_OnAlarmAdded_Callback(uint8_t list_index, uint8_t alarm_edit_h,
		uint8_t alarm_edit_m) {
	if (list_index >= 10)
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
}

void UI_OnAlarmDeleted_Callback(uint8_t list_index) {
	if (list_index >= 10)
		return;
	my_backend_alarms_array[list_index].is_active = 0; // Soft delete
}

/**
 * @brief Scanning evaluation engine iterating across all enabled software alarm blocks.
 * @details Assesses current time records against targets, updates trigger flags, and catches day transitions to roll flags back down.
 * @return True if an un-triggered active alarm threshold has been broken, False otherwise.
 */
bool check_alarm() {
	uint32_t now = current_time();
	for (int i = 0; i < 10; i++) {
		if (my_backend_alarms_array[i].is_active) {
			// Trigger matching conditions if timeline thresholds are broken
			if ((my_backend_alarms_array[i].alarm_time <= now)
					&& !(my_backend_alarms_array[i].triggered_today)) {
				my_backend_alarms_array[i].triggered_today = 1; //triggered
				return true;
			}

			// Clear structural execution locks if clock properties clear behind targets (Day turnover handling)
			else if (now < my_backend_alarms_array[i].alarm_time) {
				my_backend_alarms_array[i].triggered_today = 0;
			}
		}
	}
	return false;
}
