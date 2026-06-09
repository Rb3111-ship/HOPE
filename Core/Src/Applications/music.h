/*
 * music.h
 *
 * Created on: 21 Apr 2026
 * Author: whp27
 */

#ifndef SRC_MUSIC_H_
#define SRC_MUSIC_H_

/**
 * @brief Command enumeration flags controlling the music execution state machine.
 */
typedef enum {
	EVT_PLAY,
	EVT_RESUME,
	EVT_STOP,
	EVT_NEXT,
	EVT_PREV,
	EVT_SET_VOL,
	EVT_PAUSE,
	EVT_BLE_ON,
	EVT_BLE_OFF
}comm_type_t;

/**
 * @brief Structural message structure payload used across the FreeRTOS music queue links.
 */
typedef struct {
	comm_type_t comm;
	uint16_t data;
} music_msg_t;

#endif /* SRC_MUSIC_H_ */
