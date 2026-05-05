/*
 * DFPLAYER_driver.h
 *
 *  Created on: 29 Apr 2026
 *      Author: whp27
 */

#ifndef SRC_DRIVERS_DFPLAYER_DRIVER_H_
#define SRC_DRIVERS_DFPLAYER_DRIVER_H_


#include <stdint.h>
#include <stdbool.h>


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

void df_player_init();
void play(uint16_t track);
void pause();
void set_volume(uint8_t vol);
void change_track(uint8_t track); // if track_next go to next track else prev
void stop();
void resume();
void repeat();


#endif /* SRC_DRIVERS_DFPLAYER_DRIVER_H_ */
