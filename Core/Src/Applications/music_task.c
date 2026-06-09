/*
 * music_task.c
 *
 * Created on: 21 Apr 2026
 * Author: whp27
 */
#include "tasks.h"
#include "app_queue.h"
#include "music_service.h"
#include "music.h"

static music_msg_t msg;

/**
 * @brief FreeRTOS task handling the background audio selection execution paths.
 * @details Blocks indefinitely on musicQueueHandle until events arrive, routing commands
 * to service layer wrappers without polling variables.
 * @param pvParameters Unused FreeRTOS task parameters.
 */
void music_Task(void *pvParameters) {
	// Initialise hardware parameters before stepping into the loop block
	audio_service_init();

	for (;;) {
		// Thread enters blocked sleep state indefinitely until a new message arrives
		if (xQueueReceive(musicQueueHandle, &msg, portMAX_DELAY) == pdPASS) {

			switch (msg.comm) {
			case EVT_PLAY:
				audio_service_play(msg.data);
				break;
			case EVT_STOP:
				audio_service_stop();
				break;
			case EVT_NEXT:
				audio_service_next();
				break;
			case EVT_PREV:
				audio_service_prev();
				break;
			case EVT_SET_VOL:
				audio_service_volume(msg.data);
				break;
			case EVT_PAUSE:
				audio_service_pause();
				break;
			case EVT_RESUME:
				audio_service_resume();
				break;
			case EVT_BLE_ON:
				audio_service_ble_enable();
				break;
			case EVT_BLE_OFF:
				audio_service_ble_disable();
				break;
			}
		}

	}
}
