/*
 * music_task.c
 *
 *  Created on: 21 Apr 2026
 *      Author: whp27
 */
#include "tasks.h"
#include "app_queue.h"
#include "music_service.h"
#include "music.h"

static music_msg_t msg;
uint8_t init_flag = 0;

void music_Task(void *pvParameters) {

	for (;;) {
		if (xQueueReceive(musicQueueHandle, &msg, portMAX_DELAY) == pdPASS) {
			if (init_flag != 1) {
				audio_service_init();
				init_flag = 1;
			}

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
			case EVT_TIMER:
				audio_service_timer(msg.time);
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

//use event type to set timer
	}
}

