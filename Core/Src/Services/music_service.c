/*
 * music_service.c
 *
 *  Created on: 19 Mar 2026
 *      Author: whp27
 */
#include "music_service.h"
#include "main.h"
#include "DFPLAYER_driver.h"
#include <stdint.h>
#include "ble_driver.h"

uint8_t current_vol = 10; //add  or subtract depending on whats needed
uint8_t current_track;
// is_playing = false;
// is_ble_active = false;

void auido_service_init() {
	df_player_init();
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
void audio_service_next(){
	change_track(1);
}
void audio_service_prev(){
	change_track(0);
}

void audio_service_stop() {
	stop();
}

void audio_service_volume(uint8_t current_vol) {
	set_volume(current_vol);
}

void audio_service_ble_enable() {
	BLE_Power_On();
}
void audio_service_ble_disable() {
	BLE_Power_Off();
}

