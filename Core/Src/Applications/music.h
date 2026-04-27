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
	EVT_STOP,
	EVT_NEXT,
	EVT_PREV,
	EVT_VOL_UP,
	EVT_VOL_DOWN,
	EVT_TIMER,
	EVT_TOGGLE_PAUSE
}comm_type_t;

typedef struct{
	comm_type_t comm;
	uint16_t data;
}music_msg_t;


#endif /* SRC_MUSIC_H_ */
