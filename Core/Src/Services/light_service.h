/*
 * light_service.h
 *
 * Created on: 13 May 2026
 * Author: whp27
 */

#ifndef SRC_SERVICES_LIGHT_SERVICE_H_
#define SRC_SERVICES_LIGHT_SERVICE_H_
#include "light.h"

// Structural Service Layer Application Programming Interface Prototypes
void light_service_init();
void light_mode_moonlight();
void light_mode_starry();
void light_mode_breathing();
void light_mode_cycle();
void light_mode_torch();
void light_mode_alarm();
void light_mode_sunrise();
void light_mode_night_fade();
void light_clear();
void lightRenderer_update(light_state_t light_state);
void light_mode_reset();

#endif /* SRC_SERVICES_LIGHT_SERVICE_H_ */
