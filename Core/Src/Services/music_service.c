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

/* Global variable tracking the local current system volume context */
uint8_t current_vol = 10; //add  or subtract depending on whats needed

/**
 * @brief Handles the multi-stage hardware initialization sequence for the DFPlayer Mini.
 * @details Sends a hardware reset command and employs explicit FreeRTOS task delays to
 * respect the boot time of the chip and the mount time of the micro-SD card.
 */
void audio_service_init() {
	df_player_init();
	vTaskDelay(pdMS_TO_TICKS(1500)); // Delay 1500ms to let the DFPlayer boot up completely
	set_volume(5);
	vTaskDelay(pdMS_TO_TICKS(300));  // Delay 300ms to allow the volume command to settle
	source_select();
}

void audio_service_play(uint16_t track) {
	play(track);
}

void audio_service_resume() {
	resume();
}

void audio_service_pause() {
	pause();
}

void audio_service_next() {
	change_track(TRACK_NEXT);
}

void audio_service_prev() {
	change_track(TRACK_PREV);
}

void audio_service_stop() {
	stop();
}

void audio_service_volume(uint8_t vol) {
	current_vol = vol;
	set_volume(current_vol);
}

void audio_service_ble_enable() {
	BLE_Power_On();
}

void audio_service_ble_disable() {
	BLE_Power_Off();
}
