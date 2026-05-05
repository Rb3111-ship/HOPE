/*
 * music.h
 *
 *  Created on: 21 Apr 2026
 *      Author: whp27
 */

#ifndef SRC_MUSIC_H_
#define SRC_MUSIC_H_

typedef enum {

	EVT_PLAY,
	EVT_RESUME,
	EVT_STOP,
	EVT_NEXT,
	EVT_PREV,
	EVT_SET_VOL,
	EVT_TIMER,
	EVT_PAUSE,
	EVT_BLE_ON,
	EVT_BLE_OFF
}comm_type_t;

typedef struct{
	comm_type_t comm;
	uint16_t data;
	uint8_t time;
}music_msg_t;


#endif /* SRC_MUSIC_H_ */
