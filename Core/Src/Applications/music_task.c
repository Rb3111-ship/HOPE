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

/* The DFPlayer drops commands that arrive too close together, so leave a gap
 * after each one. Messages queue up meanwhile (queue depth 8). */
#define DF_CMD_GAP_MS          60U

/* Some modules report "track finished" twice; ignore reports that arrive this
 * soon after a (re)start so a track isn't restarted twice. */
#define FINISHED_DEBOUNCE_MS   1000U

static music_msg_t msg;

/**
 * @brief FreeRTOS task handling the background audio selection execution paths.
 * @details Blocks indefinitely on musicQueueHandle until events arrive, routing commands
 * to service layer wrappers without polling variables.
 *
 * Looping: whatever was last started with EVT_PLAY (a lullaby or the alarm tone)
 * is restarted when the module reports it finished, until EVT_STOP or Bluetooth.
 * This keeps a lullaby going for the whole sleep timer and the alarm ringing
 * until it is dismissed.
 * @param pvParameters Unused FreeRTOS task parameters.
 */
void music_Task(void *pvParameters) {
	(void) pvParameters;
	uint16_t loop_track = 0;      // 0 = nothing to loop
	uint8_t paused = 0;
	TickType_t last_start = 0;

	// Initialise hardware parameters before stepping into the loop block
	audio_service_init();

	for (;;) {
		// Thread enters blocked sleep state indefinitely until a new message arrives
		if (xQueueReceive(musicQueueHandle, &msg, portMAX_DELAY) == pdPASS) {

			switch (msg.comm) {
			case EVT_PLAY:
				audio_service_play(msg.data);
				loop_track = msg.data;
				paused = 0;
				last_start = xTaskGetTickCount();
				break;
			case EVT_STOP:
				audio_service_stop();
				loop_track = 0;
				paused = 0;
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
				paused = 1;
				break;
			case EVT_RESUME:
				audio_service_resume();
				paused = 0;
				break;
			case EVT_BLE_ON:
				loop_track = 0;
				paused = 0;
				audio_service_ble_enable();
				break;
			case EVT_BLE_OFF:
				audio_service_ble_disable();
				break;
			case EVT_TRACK_FINISHED:
				if (loop_track != 0 && !paused
						&& (xTaskGetTickCount() - last_start)
								>= pdMS_TO_TICKS(FINISHED_DEBOUNCE_MS)) {
					audio_service_play(loop_track);
					last_start = xTaskGetTickCount();
				} else {
					continue; // nothing was sent, no gap needed
				}
				break;
			}

			vTaskDelay(pdMS_TO_TICKS(DF_CMD_GAP_MS));
		}

	}
}
