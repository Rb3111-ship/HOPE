/*
 * music_service.h
 *
 *  Created on: 19 Mar 2026
 *      Author: whp27
 */

#ifndef INC_MUSIC_SERVICE_H_
#define INC_MUSIC_SERVICE_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum {
	AUDIO_EVENT_PLAY_TRACK,
	AUDIO_EVENT_PAUSE,
	AUDIO_EVENT_STOP,
	AUDIO_EVENT_NEXT,
	AUDIO_EVENT_PREV,
	AUDIO_EVENT_VOL_UP,
	AUDIO_EVENT_VOL_DOWN,
	AUDIO_EVENT_BLE_ENABLE,
	AUDIO_EVENT_BLE_DISABLE

} sys_comm_t;

bool audio_service_play(uint8_t track);
bool audio_service_pause();
bool audio_service_next();
bool audio_service_prev();
bool audio_service_init();
bool audio_service_stop();
bool audio_service_volume_up();
bool audio_service_volume_down();

bool audio_service_ble_enable();
bool audio_service_ble_disable();

#endif /* INC_MUSIC_SERVICE_H_ */
