/*
 * DFPLAYER_driver.h
 *
 * Created on: 29 Apr 2026
 * Author: whp27
 */

#ifndef SRC_DRIVERS_DFPLAYER_DRIVER_H_
#define SRC_DRIVERS_DFPLAYER_DRIVER_H_


#include <stdint.h>
#include <stdbool.h>

/* Track skipping navigation direction definitions */
#define TRACK_NEXT 1
#define TRACK_PREV 0
#define MAX_SIZE 8

/**
 * @brief Struct holding individual low-level command codes and parameters for the DFPlayer.
 */
typedef struct {
	uint8_t cmd;
	uint8_t param_high;
	uint8_t param_low;
} df_cmd_t;

/**
 * @brief Custom software ring buffer structure serializing out-going serial packets.
 */
typedef struct {
	df_cmd_t tx_buf[MAX_SIZE];
	uint8_t front;
	uint8_t rear;
	uint8_t count;
} Queue;

/* Called from the UART RX interrupt when the module reports a finished track */
typedef void (*df_finished_cb_t)(uint16_t track);

// Low-Level Hardware Driver Application Programming Interface Prototypes
void df_player_init(void);
void df_set_finished_callback(df_finished_cb_t cb);
void df_play(uint16_t track); // plays /MP3/NNNN.mp3 on the SD card
void df_pause(void);
void df_set_volume(uint8_t vol);
void df_change_track(uint8_t track); // if track_next go to next track else prev
void df_stop(void);
void df_resume(void);
void df_source_select(void);

#endif /* SRC_DRIVERS_DFPLAYER_DRIVER_H_ */
