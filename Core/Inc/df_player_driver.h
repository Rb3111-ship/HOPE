/*
 * df_player_driver.h
 *
 *  Created on: 18 Mar 2026
 *      Author: whp27
 */

#ifndef INC_DF_PLAYER_DRIVER_H_
#define INC_DF_PLAYER_DRIVER_H_

#include <stdint.h>


#define TRACK_NEXT 1
#define TRACK_PREV 0
#define MAX_SIZE 8

typedef struct {
	uint8_t cmd;
	uint8_t param_high;
	uint8_t param_low;
} df_cmd_t;

typedef struct {
	df_cmd_t tx_buf[MAX_SIZE];
	uint8_t front;
	uint8_t rear;
	uint8_t count;
} Queue;

bool df_player_init(); // send 0x0C reset
bool play(uint16_t track);
bool pause();
bool set_volume(uint8_t vol);
bool change_track(uint8_t track); // if track_next go to next track else prev
bool stop();

//check what the df player does if you press next track to the end

#endif /* INC_DF_PLAYER_DRIVER_H_ */
