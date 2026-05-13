/*
 * ui_renderer.h
 *
 *  Created on: 23 Apr 2026
 *      Author: whp27
 */

#ifndef SRC_UI_UI_RENDERER_H_
#define SRC_UI_UI_RENDERER_H_

#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "ui_state.h"
#include <stdint.h>
#include "time_service.h"
#include "alarm_service.h"

#define MAX_SONGS       25

typedef struct {
	uint8_t hours;           // 0-23  (from RTC)
	uint8_t minutes;           // 0-59  (from RTC)
	int8_t temperature;           // °C, signed (from DHT11)
	uint8_t humidity;           // % RH  (from DHT11)
	uint8_t volume;           // 0-30  (DFPlayer Mini)
} ui_render_data_t;

//if the data does not exist, does it keep previous data?
/* ═══════════════════════════════════════════════════════════════
 *  SONG LIST
 * ═══════════════════════════════════════════════════════════════
 *  HOW TO ADD YOUR SONGS:
 *    1. Replace/extend the entries below (max MAX_SONGS = 20).
 *    2. Set ui_data.song_count = your actual track count.
 *       Do this in music_manager_init() or wherever you scan the SD card.
 *
 *
 *  If you read names dynamically from the SD card, replace this static array
 *  with a char song_list[MAX_SONGS][32] buffer and fill it at runtime.
 */
const char *song_list[MAX_SONGS] = { "Twinkle Twinkle", /* index 0  0001 ---------*/
"Amazing Grace", /* index  1   0002 ------*/
"You are my sunshine", /* index  2   0003 -----------*/
"Piano",/* index  3 0004  ---------*/
"Rain And Piano", /* index  4 0005  ---------*/
/* index  5  0006*/
"Brahms Lullaby", /* index 6  0007 ---------*/
"Rock-a-bye Baby", /* index 7  0008 */
"Hush Little Baby", /* index   0009 8*/
"Frere Jacques", /* index  9 0010 */
"Row Your Boat", /* index 10  0011 */
"Baa Baa Black Sheep",/* index  0012 */
"Itsy Bitsy Spider", /* index 12   0013   ---------*/
"Wheels on the Bus", /* index  13  0014*/
"Mary Had a Lamb", /* index  14 0015*/
"Silent Night", /* index  15  0016   --------- */
"All the Pretty Little Horses", /* index  16  0017*/
"Golden Slumbers", /* index  17   0018*/
"Schubert Lullaby", /* index  18   0019*/
"Sleep Baby Sleep", /* index  19   0020*/
"Go to Sleep Little Baby", /* index  20  0021 */
"Are You Sleeping", /* index  21   0022*/
"Somewhere Over the Rainbow", /* index  22   0023*/
"Beautiful Dreamer", /* index  23   0024*/
"Summertime", /* index  24   0025*/
};

extern ui_render_data_t ui_data;

// Main renderer
void ui_renderer_update(ui_state_t state, overlay_t *overlay);

// Screen draw functions
void UI_DrawMainScreen(void);
void UI_DrawMenu(void);
void UI_DrawMusicList(void);
void UI_DrawPlayDisplay_DF(void);
void UI_DrawPlayDisplay_ble(void);
void UI_DrawTimeSetup(void);

// Overlay draw functions
void UI_DrawVolumeUp(void);
void UI_DrawVolumeDwn(void);
void UI_DrawLightsOverlay(void);
void UI_DrawTimerOverlay(void);
void UI_DrawAlarmFiringOverlay(void);

// Navigation helpers (call from button handler)
void ui_song_list_navigate(int8_t dir);           // +1 down / -1 up
void ui_menu_navigate(int8_t dir);           // +1 next icon / -1 prev
void ui_time_setup_next_field(void);           // toggle hours <-> minutes
void ui_time_setup_adjust(int8_t dir);           // +1 / -1
void ui_time_setup_get();           // read confirmed time
void ui_light_navigate(int8_t dir);
void ui_timer_navigate(int8_t dir);
int ui_get_light_mode(void);           // 0=Moonlight .. 4=Torch
int ui_get_timer_minutes(void);           // 5/10/15/30/60
void ui_nowplaying_skip(int8_t dir);
int ui_get_menu_icon(void);
void setVolume(uint8_t vol_input);
//Populate ui_render_data_t before calling ui_renderer_update
void live_data_fill(void);

// Seed the time editor with current RTC time before entering TIME_SETUP
void ui_time_setup_seed();

void ui_nowplaying_set(uint8_t index, const char *name);

void ui_nowplaying_toggle_pause(void);

uint8_t ui_get_selected_index(void);

/* API to give the UI context of your backend arrays */
void UI_SetAlarmListContext(SoftwareAlarm *alarms, uint8_t max_alarms);

/* Backend Callbacks (You must implement these in your backend task) */
extern void UI_OnAlarmDeleted_Callback(uint8_t list_index);
extern void UI_OnAlarmAdded_Callback(uint8_t list_index, uint8_t hour,
		uint8_t minute);

/* Screen & Overlay Draw Functions */
void UI_DrawTimeSubMenu(void);
void UI_DrawAlarmsList(void);
void UI_DrawAlarmSetup(void);
void UI_DrawAlarmDeleteOverlay(void);

/* Navigation Helpers for Button Handler */
void ui_time_submenu_navigate(int8_t dir);
int ui_get_time_submenu_selection(void); /* Returns: 0 = Set Time, 1 = Alarms */

void ui_alarms_list_navigate(int8_t dir);
int ui_is_selected_alarm_empty(void); /* Returns: 1 if empty, 0 if active */

void ui_alarm_setup_seed(void); /* Prepares the selected empty slot for editing */
void ui_alarm_setup_next_field(void); /* Toggles Hours/Minutes */
void ui_alarm_setup_adjust(int8_t dir); /* Increments/Decrements */
void ui_alarm_setup_confirm(void); /* Submits the alarm to backend */

void ui_alarm_delete_navigate(int8_t dir); /* Toggles YES / NO */
void ui_alarm_delete_confirm(void); /* Submits deletion to backend */


#endif /* SRC_UI_UI_RENDERER_H_ */
