/*
 * alarm_service.h
 *
 * Created on: 5 May 2026
 * Author: whp27
 */

#ifndef SRC_ALARM_SERVICE_H_
#define SRC_ALARM_SERVICE_H_
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Compound structural configuration properties managing single background software alarms.
 */
typedef struct {
	uint8_t is_active;
	uint32_t alarm_time;
	uint8_t triggered_today;
	uint8_t hour;
	uint8_t minute;
} SoftwareAlarm;

// Service Layer Application Programming Interface Prototypes
void alarm_service_init(void);
void alarm_service_load(void);          // restore alarms from EEPROM (call from a task)
void alarm_service_time_changed(void);  // call after the user sets the clock
void UI_OnAlarmDeleted_Callback(uint8_t list_index);
void UI_OnAlarmAdded_Callback(uint8_t list_index, uint8_t alarm_edit_h,
			uint8_t alarm_edit_m);
bool check_alarm(void);
#endif /* SRC_ALARM_SERVICE_H_ */
