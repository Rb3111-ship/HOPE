/*
 * ui_state.h
 *
 *  Created on: 21 Apr 2026
 *      Author: whp27
 */

#ifndef SRC_UI_UI_STATE_H_
#define SRC_UI_UI_STATE_H_
#include <stdbool.h>
#include <stdint.h>

typedef enum {

	EVT_BTN_PLAY,
	EVT_BTN_NEXT,
	EVT_BTN_PREV,
	EVT_BTN_VOL_UP,
	EVT_BTN_VOL_DOWN,
	EVT_BTN_TIMER,
	EVT_BTN_LIGHT,
	EVT_BTN_MENU,
	EVT_SENSOR_DATA_READY,
	EVT_MUSIC_FINISHED  // For when the DFPlayer naturally ends a track

} evt_type_t;

//System state
typedef enum {
	UI_STATE_MAIN,
	UI_STATE_MUSIC_MENU,
	UI_STATE_MUSIC_LIST,
	UI_STATE_NOWPLAYING_DF,
	UI_STATE_NOWPLAYING_BLE,
	UI_STATE_TIME_SETUP,
	UI_TIMER,
	UI_LIGHT_LIST,
	UI_TIMER_NOWPLAYING

} ui_state_t;

//Overlay
typedef enum {
	OVERLAY_NONE, OVERLAY_VOLUME_UP, OVERLAY_VOLUME_DOWN, OVERLAY_LIGHT_MENU, OVERLAY_TIMER
} overlay_type_t;


typedef struct {
	overlay_type_t type;
} overlay_t;


typedef struct{
	uint16_t temp;
	uint16_t humidity;
}sensor_data_t;

//Used for every command the UI has to react too
typedef struct {
    evt_type_t evt;  // Event type (e.g., enum for button press, sensor data, etc.)

    union {
        uint16_t value;        // For simple values
        sensor_data_t sensor;  // For sensor data
    } data;
} ui_msg_t;

#endif /* SRC_UI_UI_STATE_H_ */
