/*
 * music_service.h
 *
 *  Created on: 23 Apr 2026
 *      Author: whp27
 */

#ifndef SRC_SERVICES_MUSIC_SERVICE_H_
#define SRC_SERVICES_MUSIC_SERVICE_H_
#include <stdbool.h>
#include <stdint.h>
bool audio_service_play(uint16_t track);
bool audio_service_pause();
bool audio_service_next();
bool audio_service_prev();
bool audio_service_init();
bool audio_service_stop();
bool audio_service_volume_up();
bool audio_service_volume_down();

bool audio_service_ble_enable();
bool audio_service_ble_disable();

#endif /* SRC_SERVICES_MUSIC_SERVICE_H_ */
