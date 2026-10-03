/*
 * light_task.c
 *
 * Created on: 21 Apr 2026
 * Author: whp27
 */

#include "tasks.h"
#include "app_queue.h"
#include "light.h"
#include "light_service.h"
#include <stdint.h>

/**
 * @brief FreeRTOS task handling the WS2812 LED ring operating modes.
 * @details Monitors incoming light commands via lightQueueHandle. Updates the state
 * machine and refreshes the frame transformations at a steady periodic rate.
 * @param pvParameters Unused FreeRTOS task parameters.
 */
void light_Task(void *pvParameters) {
	(void) pvParameters;
	light_msg_t msg;

	// Initialize the lighting hardware driver and framework structures
	light_service_init();

	// Configure a periodic baseline timeout block of 20ms (50 Hz frame rate)
	const TickType_t xDelay20ms = pdMS_TO_TICKS(20UL);
	light_state_t currentState = OFF_STATE;

	for (;;) {
		// Non-blocking message parsing; yields for up to 20ms if queue is dry
		if (xQueueReceive(lightQueueHandle, &msg, xDelay20ms) == pdPASS) {
			switch (msg.mode) {
			case LIGHT_MOONLIGHT:
				currentState = MOONLIGHT_STATE;
				break;

			case LIGHT_STARRY:
				currentState = STARRY_STATE;
				break;

			case LIGHT_BREATHING:
				currentState = BREATHING_STATE;
				light_mode_reset(); // Reset animation variables for clean initialisation
				break;

			case LIGHT_CYCLE:
				currentState = CYCLE_STATE;
				break;

			case LIGHT_TORCH:
				currentState = TORCH_STATE;
				break;

			case LIGHT_ALARM:
				currentState = ALARM_STATE;
				light_mode_reset(); // Reset internal pulsing limits
				break;

			case LIGHT_SUNRISE:
				currentState = SUNRISE_STATE;
				light_mode_reset(); // Re-zero the sunrise phase timers
				break;

			case LIGHT_NIGHT_FADE:
				currentState = NIGHTFADE_STATE;
				light_mode_reset(); // Set baseline brightness configurations
				break;

			default:
				currentState = OFF_STATE;
				break;
			}
		}

		// Execute state calculations and render pixel buffers
		lightRenderer_update(currentState);
	}
}
