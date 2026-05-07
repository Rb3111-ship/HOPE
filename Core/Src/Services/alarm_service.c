/*
 * alarm_service.c
 *
 *  Created on: 5 May 2026
 *      Author: whp27
 */

#include "alarm_service.h"
#include "ui_renderer.h" // Include this to access UI_SetAlarmListContext
#include <string.h>
#include "time_service.h"
#include <stdbool.h>

// Your actual backend memory
SoftwareAlarm my_backend_alarms_array[10];


void alarm_service_init(void) {
	// 1. Initialise your array (set all to inactive/0)
	memset(my_backend_alarms_array, 0, sizeof(my_backend_alarms_array));

	// 2. Pass the pointer to the UI so it knows what to draw
	UI_SetAlarmListContext(my_backend_alarms_array, 10);

}

uint32_t seconds_since_midnight(uint8_t h, uint8_t m) {
	return ((h * 3600) + (m * 60));
}

uint32_t current_time() {
	uint8_t hours, mins;
	get_Time(&hours, &mins);

	return seconds_since_midnight(hours, mins);

}

void UI_OnAlarmAdded_Callback(uint8_t list_index, uint8_t alarm_edit_h,
		uint8_t alarm_edit_m) {
	my_backend_alarms_array[list_index].hour = alarm_edit_h;
	my_backend_alarms_array[list_index].minute = alarm_edit_m;
	uint32_t alarm_time = seconds_since_midnight(alarm_edit_h, alarm_edit_m);

	if (current_time() >= alarm_time) {
		my_backend_alarms_array[list_index].triggered_today = 1; //triggered today
	} else
		my_backend_alarms_array[list_index].triggered_today = 0; //not triggered

	my_backend_alarms_array[list_index].alarm_time = alarm_time;
	my_backend_alarms_array[list_index].is_active = 1; // Turn it on

}

void UI_OnAlarmDeleted_Callback(uint8_t list_index) {
	my_backend_alarms_array[list_index].is_active = 0; // Soft delete
}

bool check_alarm() {
	for (int i = 0; i < 10; i++) {
		if (my_backend_alarms_array[i].is_active) {
			if ((my_backend_alarms_array[i].alarm_time <= current_time())
					&& !(my_backend_alarms_array[i].triggered_today)) {
				my_backend_alarms_array[i].triggered_today = 1; //triggered
				return true;
			}

			else if (current_time()
					< my_backend_alarms_array[i].alarm_time) {
				my_backend_alarms_array[i].triggered_today = 0;

			}
		}
	}
	return false;
}

