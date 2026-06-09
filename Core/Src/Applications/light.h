/*
 * light.h
 *
 * Created on: 21 Apr 2026
 * Author: whp27
 */

#ifndef SRC_LIGHT_H_
#define SRC_LIGHT_H_

/**
 * @brief Upper layer commands defining specific operational modes.
 */
typedef enum {
	LIGHT_OFF,
	LIGHT_MOONLIGHT,
	LIGHT_STARRY,
	LIGHT_BREATHING,
	LIGHT_CYCLE,
	LIGHT_TORCH,
	LIGHT_SUNRISE,
	LIGHT_NIGHT_FADE,
	LIGHT_ALARM
} light_mode_t;

/**
 * @brief Local state machine enumeration mapping internal render variations.
 */
typedef enum {
	OFF_STATE,
	MOONLIGHT_STATE,
	STARRY_STATE,
	BREATHING_STATE,
	CYCLE_STATE,
	TORCH_STATE,
	ALARM_STATE,
	SUNRISE_STATE,
	NIGHTFADE_STATE
} light_state_t;

/**
 * @brief Message layout wrapper passed via the FreeRTOS lighting queue.
 */
typedef struct {
	light_mode_t mode;
} light_msg_t;

#endif /* SRC_LIGHT_H_ */
