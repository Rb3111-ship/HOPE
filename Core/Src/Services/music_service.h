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

void auido_service_init();
void audio_service_play(uint16_t track);
void audio_service_pause();
void audio_service_resume();
void audio_service_next();
void audio_service_prev();
void audio_service_init();
void audio_service_stop();
void audio_service_volume(uint8_t current_vol);
void audio_service_timer(uint8_t time); //

void audio_service_ble_enable();
void audio_service_ble_disable();

#endif /* SRC_SERVICES_MUSIC_SERVICE_H_ */
