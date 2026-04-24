/*
 * music_service.c
 *
 *  Created on: 19 Mar 2026
 *      Author: whp27
 */
#include "music_service.h"
#include "main.h"
#include "df_player_driver.h"
#include <stdint.h>
#include <stdbool.h>


uint8_t current_vol = 10;
uint8_t current_track;
bool is_playing = false;
bool is_ble_active = false;

bool audio_service_play(uint16_t track){
	if(!is_ble_active){
		if(track == current_track){
			return true;
		}
		else{
			if(!play(track)) return false;
		}
	}
	return true;
}

bool audio_service_pause(){

}
bool audio_service_next();
bool audio_service_prev();
bool audio_service_init();
bool audio_service_stop();
bool audio_service_volume_up();
bool audio_service_volume_down();

bool audio_service_ble_enable();
bool audio_service_ble_disable();
