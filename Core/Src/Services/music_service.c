/*
 * music_service.c
 *
 * Created on: 19 Mar 2026
 * Author: whp27
 */
#include "music_service.h"
#include "main.h"
#include "DFPLAYER_driver.h"
#include <stdint.h>
#include "ble_driver.h"
#include "tasks.h"
#include "app_queue.h"
#include "music.h"

/* Global variable tracking the local current system volume context */
static uint8_t current_vol = DEFAULT_VOLUME;

/**
 * @brief Runs in the UART RX interrupt: hands "track finished" to the music task.
 */
static void audio_track_finished_isr(uint16_t track) {
	BaseType_t woken = pdFALSE;
	music_msg_t msg = { .comm = EVT_TRACK_FINISHED, .data = track };
	xQueueSendFromISR(musicQueueHandle, &msg, &woken);
	portYIELD_FROM_ISR(woken);
}

/**
 * @brief Handles the multi-stage hardware initialization sequence for the DFPlayer Mini.
 * @details Sends a hardware reset command and employs explicit FreeRTOS task delays to
 * respect the boot time of the chip and the mount time of the micro-SD card.
 */
void audio_service_init(void) {
	df_set_finished_callback(audio_track_finished_isr);
	df_player_init();
	vTaskDelay(pdMS_TO_TICKS(1500)); // Delay 1500ms to let the DFPlayer boot up completely
	df_set_volume(DEFAULT_VOLUME);
	vTaskDelay(pdMS_TO_TICKS(300));  // Delay 300ms to allow the volume command to settle
	df_source_select();
}

void audio_service_play(uint16_t track) {
	df_play(track);
}

void audio_service_resume(void) {
	df_resume();
}

void audio_service_pause(void) {
	df_pause();
}

void audio_service_next(void) {
	df_change_track(TRACK_NEXT);
}

void audio_service_prev(void) {
	df_change_track(TRACK_PREV);
}

void audio_service_stop(void) {
	df_stop();
}

void audio_service_volume(uint8_t vol) {
	current_vol = vol;
	df_set_volume(current_vol);
}

void audio_service_ble_enable(void) {
	df_stop();                       // never play SD audio and Bluetooth audio together
	vTaskDelay(pdMS_TO_TICKS(50));
	BLE_Power_On();
}

void audio_service_ble_disable(void) {
	BLE_Power_Off();
}
