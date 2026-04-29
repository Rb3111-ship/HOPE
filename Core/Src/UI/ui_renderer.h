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
static const char *song_list[MAX_SONGS] = { "Twinkle Twinkle", /* index 0  */
"Amazing Grace", /* index  1 */
"You are my sunshine", /* index  2 */
"Jesu idombo", /* index  3 */
"Hareruya kuna Jesu", /* index  4 */
"Mekeniki Manyeruke", /* index 5  */
"Brahms Lullaby", /* index 6 */
"Rock-a-bye Baby", /* index 7  */
"Hush Little Baby", /* index   8*/
"Frere Jacques", /* index  9 */
"Row Your Boat", /* index 10  */
"Baa Baa Black Sheep",/* index  11*/
"Itsy Bitsy Spider", /* index 12  */
"Wheels on the Bus", /* index  13 */
"Mary Had a Lamb", /* index  14 */
"Silent Night", /* index  15 */
"All the Pretty Little Horses", /* index  16 */
"Golden Slumbers", /* index  17 */
"Schubert Lullaby", /* index  18 */
"Sleep Baby Sleep", /* index  19 */
"Go to Sleep Little Baby", /* index  20 */
"Are You Sleeping", /* index  21 */
"Somewhere Over the Rainbow", /* index  22 */
"Beautiful Dreamer", /* index  23 */
"Summertime", /* index  24 */
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

//Populate ui_render_data_t before calling ui_renderer_update
void live_data_fill(void);

// Seed the time editor with current RTC time before entering TIME_SETUP
void ui_time_setup_seed(uint8_t h, uint8_t m);

void ui_nowplaying_set(uint8_t index, const char *name);

void ui_nowplaying_toggle_pause(void);

uint8_t ui_get_selected_index(void);

#endif /* SRC_UI_UI_RENDERER_H_ */
