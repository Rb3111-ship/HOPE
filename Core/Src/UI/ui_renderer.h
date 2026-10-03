/*
 * ui_renderer.h
 *
 * Created on: 23 Apr 2026
 * Author: whp27
 */

#ifndef SRC_UI_UI_RENDERER_H_
#define SRC_UI_UI_RENDERER_H_

#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "ui_state.h"
#include <stdint.h>
#include "time_service.h"
#include "alarm_service.h"

/* Maximum number of tracks supported by the UI system */
#define MAX_SONGS       25

/* * Structure holding the localized data needed to populate the display.
 * This decouples the renderer from the raw driver files.
 */
typedef struct {
	uint8_t hours;           // 0-23  (from RTC)
	uint8_t minutes;           // 0-59  (from RTC)
	int8_t temperature;           // °C, signed (from DHT22)
	uint8_t humidity;           // % RH  (from DHT22)
	uint8_t sensor_valid;           // 0 until the DHT22 has given one good reading
	uint8_t volume;           // 0-30  (DFPlayer Mini)
} ui_render_data_t;

/* Global shared UI display structure */
extern ui_render_data_t ui_data;
/* Fixed string matrix containing track names populated on the SD card */
extern const char * song_list[MAX_SONGS];

// Main renderer
/* Top-level execution call that clears the buffer, selects views/overlays, and flushes to OLED */
void ui_renderer_update(ui_state_t state, overlay_t *overlay);

// Screen draw functions
/* Individual window drawing primitives for each structural app state */
void UI_DrawMainScreen(void);
void UI_DrawMenu(void);
void UI_DrawMusicList(void);
void UI_DrawPlayDisplay_DF(void);
void UI_DrawPlayDisplay_ble(void);
void UI_DrawTimeSetup(void);

// Overlay draw functions
/* Temporary popup sub-windows layered on top of structural states */
void UI_DrawVolumeUp(void);
void UI_DrawVolumeDwn(void);
void UI_DrawLightsOverlay(void);
void UI_DrawTimerOverlay(void);
void UI_DrawAlarmFiringOverlay(void);

// Navigation helpers (call from button handler)
/* Selection and view-bounding modifiers called on button events */
void ui_song_list_navigate(int8_t dir);           // +1 down / -1 up
void ui_menu_navigate(int8_t dir);           // +1 next icon / -1 prev
void ui_time_setup_next_field(void);           // toggle hours <-> minutes
void ui_time_setup_adjust(int8_t dir);           // +1 / -1
void ui_time_setup_get(void);           // read confirmed time
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
void ui_time_setup_seed(void);

/* Configures the local cache string and attributes when tracking a running track */
void ui_nowplaying_set(uint8_t index, const char *name);

/* Alternates the local playback play/pause symbol state */
void ui_nowplaying_toggle_pause(void);

/* Sets the play/pause symbol explicitly (1 = playing, 0 = paused) */
void ui_nowplaying_set_playing(uint8_t playing);

/* Index (0-based) of the song currently shown on the player screen */
uint8_t ui_nowplaying_get_index(void);

/* Returns the current active list cursor configuration pointer */
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
void ui_alarm_delete_cancel(void); /* Closes prompt without deleting */


#endif /* SRC_UI_UI_RENDERER_H_ */
