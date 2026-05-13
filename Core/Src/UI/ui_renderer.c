/*
 * ui_renderer.c
 *
 *  Created on: 21 Apr 2026
 *      Author: whp27
 */
#include "ui_renderer.h"
#include "ui_state.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "sensor_service.h"
#include "tasks.h"
#include "cmsis_os.h"
#include "alarm_service.h"

/* ═══════════════════════════════════════════════════════════════
 *  CONSTANTS
 * ═══════════════════════════════════════════════════════════════ */

#define DISPLAY_W       128u
#define DISPLAY_H       128u
#define VOLUME_MAX      30          /* DFPlayer Mini max volume */
#define LIST_ROW_H      11          /* Pixels per list row (Font_6x8 + 3px gap) */
#define LIST_VISIBLE    10          /* Max visible rows in scrollable lists */

/* Animation parameters */
#define ANIM_TICK_MAX   240u        /* Animation loop counter (10Hz: 240 ticks = 24s loop) */

const uint8_t song_count = 25;
float sensor_data[2];
uint8_t volume = 0;

/* ═══════════════════════════════════════════════════════════════
 *  LIVE DATA
 * ═══════════════════════════════════════════════════════════════ */

void setVolume(uint8_t vol_input) {
	volume = vol_input;
}

/* Fetches external state to populate UI data. Call before UI updates. */
void live_data_fill() {
	static uint32_t last_sensor_read = 0;
	uint32_t now = xTaskGetTickCount();

	get_Time(&ui_data.hours, &ui_data.minutes);

	/* Throttle sensor reads to 5-second intervals */
	if ((now - last_sensor_read) >= pdMS_TO_TICKS(5000)) {
		last_sensor_read = now;
		get_sensor_Data(sensor_data);
		ui_data.humidity = sensor_data[0];
		ui_data.temperature = sensor_data[1];
	}

	ui_data.volume = volume;
}

/* ═══════════════════════════════════════════════════════════════
 *  PRIVATE STATE
 * ═══════════════════════════════════════════════════════════════ */

static uint32_t anim_tick = 0; /* Global animation frame counter */

/* Navigation State */
static int music_scroll = 0; /* Top visible song index */
static int music_selected = 0; /* Highlighted song index */
static int menu_icon = 0; /* 0=Music, 1=Bluetooth, 2=Time */

/* Overlay State */
static int light_selected = 0; /* 0-4 options */
static int timer_selected = 0; /* 0-4 options */

/* Time Editor State */
static uint8_t time_field = 0; /* 0=Hours, 1=Minutes */
static uint8_t time_h = 0;
static uint8_t time_m = 0;

/* Marquee Scroll State (For long track names) */
static int song_scroll_offset = 0;
static int song_scroll_pause = 0;
static uint32_t song_scroll_last = 0;

/* Now Playing Cache (Written on selection, read by renderer) */
static char np_song_name[32] = "---";
static uint8_t np_song_index = 0;
static uint8_t np_is_playing = 0;

/* Alarm System State */
static SoftwareAlarm *backend_alarms = NULL;
static uint8_t backend_max_alarms = 0;

static int time_submenu_selected = 0; /* 0: Set Time, 1: Alarms */
static int alarms_scroll = 0;
static int alarms_selected = 0;
static int alarm_delete_choice = 1; /* 0: YES, 1: NO (Safe default) */

static uint8_t alarm_edit_field = 0; /* 0: Hours, 1: Minutes */
static uint8_t alarm_edit_h = 0;
static uint8_t alarm_edit_m = 0;
/* ═══════════════════════════════════════════════════════════════
 *  NAVIGATION HELPERS
 * ═══════════════════════════════════════════════════════════════ */

/* Navigate music list: dir = +1 (down) or -1 (up) */
void ui_song_list_navigate(int8_t dir) {
	int count = (song_count > 0) ? (int) song_count : 1;
	music_selected += dir;
	if (music_selected < 0)
		music_selected = 0;
	if (music_selected >= count)
		music_selected = count - 1;

	/* Update scroll window to follow selection */
	if (music_selected < music_scroll)
		music_scroll = music_selected;
	if (music_selected >= music_scroll + LIST_VISIBLE)
		music_scroll = music_selected - LIST_VISIBLE + 1;
}

/* Cycle main menu: 0->1->2->0 */
void ui_menu_navigate(int8_t dir) {
	menu_icon = (menu_icon + (int) dir + 3) % 3;
}

/* Time Setup Navigation */
void ui_time_setup_next_field(void) {
	time_field ^= 1u;
}

void ui_time_setup_adjust(int8_t dir) {
	if (time_field == 0)
		time_h = (uint8_t) ((time_h + (int) dir + 24) % 24);
	else
		time_m = (uint8_t) ((time_m + (int) dir + 60) % 60);
}

/* Pre-load time editor with current RTC time */
void ui_time_setup_seed() {
	time_h = ui_data.hours % 24u;
	time_m = ui_data.minutes % 60u;
	time_field = 0;
}

void ui_time_setup_get() {
	set_TimeMins(time_m);
	set_TimeH(time_h);
	confirm_time();
}

/* Overlay Navigation */
void ui_light_navigate(int8_t dir) {
	light_selected = (light_selected + (int) dir + 6) % 6;
}

void ui_timer_navigate(int8_t dir) {
	timer_selected = (timer_selected + (int) dir + 6) % 6;
}

/* Returns: 0=Moonlight, 1=Starry, 2=Breathing, 3=Cycle, 4=Torch */
int ui_get_light_mode(void) {
	return light_selected;
}

/* Returns confirmed timer duration in minutes */
int ui_get_timer_minutes(void) {
	static const int timers[6] = { 0, 5, 10, 15, 30, 60 };
	return timers[timer_selected];
}

/* Now Playing Handlers */
void ui_nowplaying_set(uint8_t index, const char *name) {
	np_song_index = index;
	strncpy(np_song_name, name, sizeof(np_song_name) - 1);
	np_song_name[sizeof(np_song_name) - 1] = '\0';
	np_is_playing = 1;

	/* Reset marquee scroll for new song */
	song_scroll_offset = 0;
	song_scroll_pause = 0;
	song_scroll_last = anim_tick;
}

void ui_nowplaying_toggle_pause(void) {
	np_is_playing ^= 1u;
}

uint8_t ui_get_selected_index(void) {
	return (uint8_t) music_selected;
}

/* Handle skip in UI_STATE_NOWPLAYING_DF. Keeps list cursor in sync. */
void ui_nowplaying_skip(int8_t dir) {
	int count = (song_count > 0) ? (int) song_count : 1;
	int next = ((int) np_song_index + (int) dir + count) % count;

	music_selected = next;
	if (music_selected < music_scroll)
		music_scroll = music_selected;
	if (music_selected >= music_scroll + LIST_VISIBLE)
		music_scroll = music_selected - LIST_VISIBLE + 1;

	ui_nowplaying_set((uint8_t) next, song_list[next]);
}

/* Alarm Context Setup */
void UI_SetAlarmListContext(SoftwareAlarm *alarms, uint8_t max_alarms) {
	backend_alarms = alarms;
	backend_max_alarms = max_alarms > 10 ? 10 : max_alarms; /* Constrain to 10 max slots */
}

/* Time Submenu Navigation */
void ui_time_submenu_navigate(int8_t dir) {
	time_submenu_selected = (time_submenu_selected + (int) dir + 2) % 2;
}

int ui_get_time_submenu_selection(void) {
	return time_submenu_selected;
}

/* Alarms List Navigation */
void ui_alarms_list_navigate(int8_t dir) {
	int count = (int) backend_max_alarms;
	if (count == 0)
		return;

	alarms_selected += dir;
	if (alarms_selected < 0)
		alarms_selected = 0;
	if (alarms_selected >= count)
		alarms_selected = count - 1;

	if (alarms_selected < alarms_scroll)
		alarms_scroll = alarms_selected;
	if (alarms_selected >= alarms_scroll + LIST_VISIBLE)
		alarms_scroll = alarms_selected - LIST_VISIBLE + 1;
}

int ui_is_selected_alarm_empty(void) {
	if (backend_alarms == NULL || backend_max_alarms == 0)
		return 1;
	return (backend_alarms[alarms_selected].is_active == 0) ? 1 : 0;
}

/* Alarm Setup Navigation */
void ui_alarm_setup_seed(void) {
	alarm_edit_h = 8; /* Default empty slot to 08:00 */
	alarm_edit_m = 0;
	alarm_edit_field = 0;
}

void ui_alarm_setup_next_field(void) {
	alarm_edit_field ^= 1u;
}

void ui_alarm_setup_adjust(int8_t dir) {
	if (alarm_edit_field == 0)
		alarm_edit_h = (uint8_t) ((alarm_edit_h + (int) dir + 24) % 24);
	else
		alarm_edit_m = (uint8_t) ((alarm_edit_m + (int) dir + 60) % 60);
}

void ui_alarm_setup_confirm(void) {
	/* Fire callback to save alarm configuration */

	UI_OnAlarmAdded_Callback((uint8_t) alarms_selected, alarm_edit_h,
			alarm_edit_m);
}

/* Alarm Delete Navigation */
void ui_alarm_delete_navigate(int8_t dir) {
	alarm_delete_choice = (alarm_delete_choice + (int) dir + 2) % 2;
}

void ui_alarm_delete_confirm(void) {
	if (alarm_delete_choice == 0) { /* YES selected */
		UI_OnAlarmDeleted_Callback((uint8_t) alarms_selected);
	}
	alarm_delete_choice = 1; /* Reset to safe choice */
}

/* ═══════════════════════════════════════════════════════════════
 *  PRIVATE DRAWING HELPERS
 * ═══════════════════════════════════════════════════════════════ */

static inline uint8_t clamp8(int v) {
	if (v < 0)
		return 0u;
	if (v > 127)
		return 127u;
	return (uint8_t) v;
}

/* 5-pointed star (R=8, r=4) */
static const int8_t STAR_X[11] = { 0, 2, 8, 4, 5, 0, -5, -4, -8, -2, 0 };
static const int8_t STAR_Y[11] = { -8, -3, -2, 1, 6, 4, 6, 1, -2, -3, -8 };

static void draw_star(int cx, int cy, SSD1306_COLOR col) {
	SSD1306_VERTEX pts[11];
	for (int i = 0; i < 11; i++) {
		pts[i].x = clamp8(cx + STAR_X[i]);
		pts[i].y = clamp8(cy + STAR_Y[i]);
	}
	ssd1306_Polyline(pts, 11, col);
}

/* Small star (R=4, r=2) */
static const int8_t SSTAR_X[11] = { 0, 1, 4, 2, 2, 0, -2, -2, -4, -1, 0 };
static const int8_t SSTAR_Y[11] = { -4, -1, -1, 1, 3, 2, 3, 1, -1, -1, -4 };

static void draw_small_star(int cx, int cy, SSD1306_COLOR col) {
	SSD1306_VERTEX pts[11];
	for (int i = 0; i < 11; i++) {
		pts[i].x = clamp8(cx + SSTAR_X[i]);
		pts[i].y = clamp8(cy + SSTAR_Y[i]);
	}
	ssd1306_Polyline(pts, 11, col);
}

/* Single music note */
static void draw_music_note(int x, int y, SSD1306_COLOR col) {
	if (y < 0 || y > 119 || x < 0 || x > 117)
		return;
	ssd1306_Line(clamp8(x + 5), clamp8(y), clamp8(x + 5), clamp8(y + 8), col);
	ssd1306_FillCircle(clamp8(x + 3), clamp8(y + 8), 2, col);
	ssd1306_Line(clamp8(x + 5), clamp8(y), clamp8(x + 9), clamp8(y + 3), col);
}

/* Animated bunny handler (frame: 0-3) */
static void draw_bunny(int cx, int head_top_y, uint8_t frame) {
	/* Ears */
	ssd1306_Line(clamp8(cx - 5), clamp8(head_top_y + 10), clamp8(cx - 6),
			clamp8(head_top_y), White);
	ssd1306_Line(clamp8(cx - 3), clamp8(head_top_y + 10), clamp8(cx - 5),
			clamp8(head_top_y), White);
	ssd1306_Line(clamp8(cx + 3), clamp8(head_top_y + 10), clamp8(cx + 5),
			clamp8(head_top_y), White);
	ssd1306_Line(clamp8(cx + 5), clamp8(head_top_y + 10), clamp8(cx + 6),
			clamp8(head_top_y), White);

	/* Head */
	int hcy = head_top_y + 14;
	ssd1306_FillCircle(clamp8(cx), clamp8(hcy), 6, White);
	ssd1306_DrawPixel(clamp8(cx - 2), clamp8(hcy - 1), Black);
	ssd1306_DrawPixel(clamp8(cx + 2), clamp8(hcy - 1), Black);
	ssd1306_DrawPixel(clamp8(cx), clamp8(hcy + 1), Black);

	/* Body */
	int bcy = hcy + 14;
	ssd1306_FillCircle(clamp8(cx), clamp8(bcy), 8, White);

	/* Arms - swing up on frames 2-3 */
	int arm_dy = (frame >= 2) ? -3 : 2;
	ssd1306_Line(clamp8(cx - 8), clamp8(bcy - 2), clamp8(cx - 14),
			clamp8(bcy - 2 + arm_dy), White);
	ssd1306_Line(clamp8(cx + 8), clamp8(bcy - 2), clamp8(cx + 14),
			clamp8(bcy - 2 - arm_dy), White);

	/* Legs - alternate hop */
	int ll = ((frame == 1) || (frame == 3)) ? 3 : 0;
	int rl = ((frame == 0) || (frame == 2)) ? 3 : 0;
	int ly = bcy + 7;
	ssd1306_Line(clamp8(cx - 3), clamp8(ly), clamp8(cx - 5),
			clamp8(ly + 6 + ll), White);
	ssd1306_Line(clamp8(cx + 3), clamp8(ly), clamp8(cx + 5),
			clamp8(ly + 6 + rl), White);

	/* Feet */
	ssd1306_Line(clamp8(cx - 5), clamp8(ly + 6 + ll), clamp8(cx - 9),
			clamp8(ly + 6 + ll), White);
	ssd1306_Line(clamp8(cx + 5), clamp8(ly + 6 + rl), clamp8(cx + 9),
			clamp8(ly + 6 + rl), White);
}

/* Small Bluetooth icon (~10x12) */
static void draw_bt_icon(int x, int y, SSD1306_COLOR col) {
	ssd1306_Line(clamp8(x + 3), clamp8(y), clamp8(x + 3), clamp8(y + 11), col);
	ssd1306_Line(clamp8(x + 3), clamp8(y), clamp8(x + 7), clamp8(y + 3), col);
	ssd1306_Line(clamp8(x + 7), clamp8(y + 3), clamp8(x + 3), clamp8(y + 6),
			col);
	ssd1306_Line(clamp8(x + 3), clamp8(y + 6), clamp8(x + 7), clamp8(y + 9),
			col);
	ssd1306_Line(clamp8(x + 7), clamp8(y + 9), clamp8(x + 3), clamp8(y + 11),
			col);
	ssd1306_Line(clamp8(x + 3), clamp8(y + 3), clamp8(x), clamp8(y), col);
	ssd1306_Line(clamp8(x + 3), clamp8(y + 9), clamp8(x), clamp8(y + 11), col);
}

/* Large Menu Bluetooth Icon */
static void draw_large_bt_icon(int cx, int cy) {
	int x = cx - 7;
	int y = cy - 17;
	ssd1306_Line(clamp8(x + 7), clamp8(y), clamp8(x + 7), clamp8(y + 34),
			White);
	ssd1306_Line(clamp8(x + 7), clamp8(y), clamp8(x + 14), clamp8(y + 8),
			White);
	ssd1306_Line(clamp8(x + 14), clamp8(y + 8), clamp8(x + 7), clamp8(y + 17),
			White);
	ssd1306_Line(clamp8(x + 7), clamp8(y + 17), clamp8(x + 14), clamp8(y + 26),
			White);
	ssd1306_Line(clamp8(x + 14), clamp8(y + 26), clamp8(x + 7), clamp8(y + 34),
			White);
	ssd1306_Line(clamp8(x + 7), clamp8(y + 8), clamp8(x), clamp8(y), White);
	ssd1306_Line(clamp8(x + 7), clamp8(y + 26), clamp8(x), clamp8(y + 34),
			White);
}

/* Large Menu Music Note */
static void draw_large_music_note(int x, int y) {
	ssd1306_Line(clamp8(x + 10), clamp8(y), clamp8(x + 10), clamp8(y + 22),
			White);
	ssd1306_FillCircle(clamp8(x + 6), clamp8(y + 22), 7, White);
	ssd1306_FillCircle(clamp8(x + 6), clamp8(y + 22), 3, Black);
	ssd1306_Line(clamp8(x + 10), clamp8(y), clamp8(x + 22), clamp8(y + 8),
			White);
	ssd1306_Line(clamp8(x + 22), clamp8(y + 8), clamp8(x + 10), clamp8(y + 13),
			White);
}

/* Menu Clock Face Icon */
static void draw_clock_icon(int cx, int cy, int r) {
	ssd1306_DrawCircle(clamp8(cx), clamp8(cy), (uint8_t) r, White);
	ssd1306_Line(clamp8(cx), clamp8(cy), clamp8(cx - 4), clamp8(cy - r + 3),
			White);
	ssd1306_Line(clamp8(cx), clamp8(cy), clamp8(cx + r / 2), clamp8(cy - r / 2),
			White);
	ssd1306_FillCircle(clamp8(cx), clamp8(cy), 2, White);
	ssd1306_Line(clamp8(cx), clamp8(cy - r), clamp8(cx), clamp8(cy - r + 3),
			White);
	ssd1306_Line(clamp8(cx + r), clamp8(cy), clamp8(cx + r - 3), clamp8(cy),
			White);
	ssd1306_Line(clamp8(cx), clamp8(cy + r), clamp8(cx), clamp8(cy + r - 3),
			White);
	ssd1306_Line(clamp8(cx - r), clamp8(cy), clamp8(cx - r + 3), clamp8(cy),
			White);
}

/* Horizontal Volume Bar */
static void draw_vol_bar(uint8_t x1, uint8_t y1, uint8_t width, uint8_t height,
		uint8_t vol) {
	ssd1306_DrawRectangle(x1, y1, x1 + width, y1 + height, White);
	if (vol > 0 && width > 2u) {
		uint8_t fw = (uint8_t) ((uint16_t) vol * (width - 2u) / VOLUME_MAX);
		if (fw > 0u)
			ssd1306_FillRectangle(x1 + 1u, y1 + 1u, x1 + 1u + fw,
					y1 + height - 1u, White);
	}
}

/* List Scrollbar Component */
static void draw_scrollbar(uint8_t x, uint8_t y, uint8_t track_h, int total,
		int visible, int top) {
	ssd1306_DrawRectangle(x, y, x + 3u, y + track_h, White);
	if (total <= visible)
		return;

	uint8_t th = (uint8_t) ((uint16_t) visible * track_h / (uint16_t) total);
	if (th < 4u)
		th = 4u;

	uint8_t ty = (uint8_t) (y
			+ (uint32_t) top * (track_h - th) / (uint32_t) (total - visible));
	ssd1306_FillRectangle(x + 1u, ty, x + 2u, ty + th, White);
}

/* Helper to center strings horizontally based on font width */
static void draw_centered_str(const char *str, SSD1306_Font_t font,
		uint8_t char_w, uint8_t y) {
	uint8_t len = (uint8_t) strlen(str);
	uint8_t total_w = len * char_w;
	uint8_t x = (total_w < DISPLAY_W) ? (DISPLAY_W - total_w) / 2u : 0u;
	ssd1306_SetCursor(x, y);
	ssd1306_WriteString((char*) str, font, White);
}

/* Generic List Renderer */
static void draw_list(const char **items, int count, int selected, int top,
		uint8_t list_y, uint8_t list_h, uint8_t list_w) {
	int visible = list_h / LIST_ROW_H;
	for (int i = 0; i < visible; i++) {
		int idx = top + i;
		if (idx >= count)
			break;

		uint8_t row_y = list_y + (uint8_t) (i * LIST_ROW_H);
		if (idx == selected) {
			ssd1306_FillRectangle(0u, row_y, list_w, row_y + LIST_ROW_H - 1u,
					White);
			ssd1306_SetCursor(3u, row_y + 2u);
			ssd1306_WriteString((char*) items[idx], Font_6x8, Black);
		} else {
			ssd1306_SetCursor(3u, row_y + 2u);
			ssd1306_WriteString((char*) items[idx], Font_6x8, White);
		}
	}
	draw_scrollbar(list_w + 2u, list_y, list_h, count, visible, top);
}

/* Core Animation Scene: Renders Bunny, floating stars, drifting note */
static void draw_anim_scene(uint8_t zone_y, uint8_t zone_h) {
	static const int8_t hop[8] = { 0, -3, -6, -8, -6, -3, 0, 0 };

	uint8_t anim_step = (uint8_t) ((anim_tick / 6u) % 8u);
	uint8_t draw_frm = (uint8_t) ((anim_tick / 6u) % 4u);

	int bunny_cx = 30;
	int bunny_ground = (int) zone_y + (int) zone_h - 38;
	int bunny_top_y = bunny_ground + hop[anim_step];
	draw_bunny(bunny_cx, bunny_top_y, draw_frm);

	ssd1306_Line(4u, zone_y + zone_h - 2u, 123u, zone_y + zone_h - 2u, White);

	/* Render Floating stars */
	{
		static const int sx[3] = { 72, 92, 112 };
		static const uint8_t periods[3] = { 26u, 32u, 22u };
		for (int s = 0; s < 3; s++) {
			uint8_t period = periods[s];
			uint32_t phase = (uint32_t) (s * (ANIM_TICK_MAX / 3));
			uint8_t t = (uint8_t) ((anim_tick + phase) % period);
			int sy = ((int) zone_y + (int) zone_h - 8)
					- (int) ((uint32_t) t * zone_h / period);
			if (sy >= (int) zone_y && sy < (int) (zone_y + zone_h)) {
				if (s == 0)
					draw_star(sx[s], sy, White);
				else
					draw_small_star(sx[s], sy, White);
			}
		}
	}

	/* Render Floating music note */
	{
		uint8_t t = (uint8_t) (anim_tick % 38u);
		int ny = ((int) zone_y + (int) zone_h - 12)
				- (int) ((uint32_t) t * zone_h / 38u);
		if (ny >= (int) zone_y)
			draw_music_note(52, ny, White);
	}
}

/* Base Volume Overlay (+1: Vol Up, -1: Vol Down) */
static void draw_vol_overlay_body(int direction) {
	ssd1306_FillRectangle(18u, 30u, 109u, 117u, Black);
	ssd1306_DrawRectangle(18u, 30u, 109u, 117u, White);

	draw_centered_str("VOLUME", Font_7x10, 7u, 33u);
	ssd1306_Line(18u, 44u, 109u, 44u, White);

	/* Draw Speaker Cone */
	ssd1306_DrawRectangle(26u, 55u, 38u, 73u, White);
	{
		SSD1306_VERTEX cone[4];
		cone[0].x = 38u;
		cone[0].y = 55u;
		cone[1].x = 52u;
		cone[1].y = 46u;
		cone[2].x = 52u;
		cone[2].y = 82u;
		cone[3].x = 38u;
		cone[3].y = 73u;
		ssd1306_Polyline(cone, 4u, White);
	}

	/* Draw Sound Waves */
	ssd1306_DrawArc(38u, 64u, 9u, 300u, 120u, White);
	ssd1306_DrawArc(38u, 64u, 15u, 300u, 120u, White);

	/* Draw +/- indicator */
	ssd1306_DrawCircle(85u, 64u, 13u, White);
	ssd1306_Line(79u, 64u, 91u, 64u, White);
	if (direction > 0)
		ssd1306_Line(85u, 58u, 85u, 70u, White);

	{
		char buf[8];
		snprintf(buf, sizeof(buf), "%d", (int) ui_data.volume);
		draw_centered_str(buf, Font_11x18, 11u, 82u);
	}

	draw_vol_bar(22u, 103u, 85u, 8u, ui_data.volume);
}


/* Alarm bell icon (~22 wide, 20 tall, centred on cx,cy) */
static void draw_alarm_bell(int cx, int cy, SSD1306_COLOR col) {
    /* Dome */
    ssd1306_DrawArc(clamp8(cx), clamp8(cy + 2), 10u, 180u, 360u, col);
    /* Sides drop down from dome rim */
    ssd1306_Line(clamp8(cx - 10), clamp8(cy + 2),
                 clamp8(cx - 10), clamp8(cy + 9), col);
    ssd1306_Line(clamp8(cx + 10), clamp8(cy + 2),
                 clamp8(cx + 10), clamp8(cy + 9), col);
    /* Base bar */
    ssd1306_Line(clamp8(cx - 12), clamp8(cy + 9),
                 clamp8(cx + 12), clamp8(cy + 9), col);
    /* Clapper dot */
    ssd1306_FillCircle(clamp8(cx), clamp8(cy + 13), 2u, col);
    /* Stem + handle */
    ssd1306_Line(clamp8(cx), clamp8(cy - 8),
                 clamp8(cx), clamp8(cy - 12), col);
    ssd1306_Line(clamp8(cx - 3), clamp8(cy - 12),
                 clamp8(cx + 3), clamp8(cy - 12), col);
}

/* Returns 1 if alarm content should be drawn this tick (fade simulation) */
static uint8_t alarm_fade_visible(void) {
    uint8_t t = (uint8_t)(anim_tick % 60u);
    if (t < 20u) return 1u;                         /* Fully on         */
    if (t < 28u) return (uint8_t)(t % 2u);          /* 50% — fading out */
    if (t < 36u) return (uint8_t)(t % 4u == 0u);    /* 25% — near off   */
    if (t < 40u) return 0u;                          /* Fully off        */
    if (t < 44u) return (uint8_t)(t % 4u == 0u);    /* 25% — fading in  */
    if (t < 52u) return (uint8_t)(t % 2u);          /* 50% — fading in  */
    return 1u;                                        /* Fully on         */
}

/* Alarm Firing Overlay: full-screen, time + bell, fades in/out */
void UI_DrawAlarmFiringOverlay(void) {
    char buf[8];

    /* Full-screen dark box with border */
    ssd1306_FillRectangle(0u, 0u, 127u, 127u, Black);
    ssd1306_DrawRectangle(0u, 0u, 127u, 127u, White);

    /* Header bar */
    ssd1306_FillRectangle(0u, 0u, 127u, 16u, White);
    draw_centered_str("  ALARM  ", Font_7x10, 7u, 3u);

    if (alarm_fade_visible()) {
        /* Large time display */
    	snprintf(buf, sizeof(buf), "%02d:%02d",
    	         (int)ui_data.hours, (int)ui_data.minutes);
        ssd1306_SetCursor(14u, 30u);
        ssd1306_WriteString(buf, Font_16x26, White);

        /* Animated bell — swings left/right using anim_tick */
        int bell_cx = 64 + (int)((anim_tick % 10u < 5u) ? 4 : -4);
        draw_alarm_bell(bell_cx, 82, White);
    }

    /* Dismiss hint — always visible so user knows what to do */
    ssd1306_Line(0u, 112u, 127u, 112u, White);
    ssd1306_SetCursor(10u, 118u);
    ssd1306_WriteString("Press OK to dismiss", Font_6x8, White);
}



/* ═══════════════════════════════════════════════════════════════
 *  SCREEN IMPLEMENTATIONS
 * ═══════════════════════════════════════════════════════════════ */

/* Main Screen: Shows Clock, Temp/Hum Sensor data, Anim Scene, Volume */
void UI_DrawMainScreen(void) {
	char buf[32];

	snprintf(buf, sizeof(buf), "%02d:%02d", ui_data.hours, ui_data.minutes);
	ssd1306_SetCursor(36u, 2u);
	ssd1306_WriteString(buf, Font_11x18, White);

	ssd1306_Line(0u, 21u, 127u, 21u, White);

	snprintf(buf, sizeof(buf), " %dC   Hum:%d%%", (int) ui_data.temperature,
			(int) ui_data.humidity);
	ssd1306_SetCursor(2u, 24u);
	ssd1306_WriteString(buf, Font_6x8, White);

	ssd1306_Line(0u, 33u, 127u, 33u, White);

	draw_anim_scene(34u, 72u);

	ssd1306_Line(0u, 106u, 127u, 106u, White);

	ssd1306_SetCursor(2u, 110u);
	ssd1306_WriteString("VOL", Font_6x8, White);
	draw_vol_bar(26u, 110u, 98u, 8u, ui_data.volume);
}

int ui_get_menu_icon(void) {
	return menu_icon;
}

/* Menu Screen: Large Icon Carousel (0:Music, 1:BT, 2:Time) */
void UI_DrawMenu(void) {
	static const char *labels[3] = { "MUSIC", "BLUETOOTH", "TIME" };

	/* Render left navigation arrow */
	if (menu_icon > 0) {
		ssd1306_Line(7u, 64u, 13u, 58u, White);
		ssd1306_Line(7u, 64u, 13u, 70u, White);
		ssd1306_Line(7u, 64u, 13u, 64u, White);
	}

	/* Render right navigation arrow */
	if (menu_icon < 2) {
		ssd1306_Line(120u, 64u, 114u, 58u, White);
		ssd1306_Line(120u, 64u, 114u, 70u, White);
		ssd1306_Line(120u, 64u, 114u, 64u, White);
	}

	switch (menu_icon) {
	case 0:
		draw_large_music_note(42, 38);
		break;
	case 1:
		draw_large_bt_icon(64, 62);
		break;
	case 2:
		draw_clock_icon(64, 60, 28);
		break;
	}

	draw_centered_str(labels[menu_icon], Font_7x10, 7u, 106u);

	/* Render pagination dots */
	for (int i = 0; i < 3; i++) {
		uint8_t dx = (uint8_t) (55 + i * 9);
		if (i == menu_icon)
			ssd1306_FillCircle(dx, 121u, 3u, White);
		else
			ssd1306_DrawCircle(dx, 121u, 3u, White);
	}
}

/* Music List: Scrollable track selection view */
void UI_DrawMusicList(void) {
	ssd1306_FillRectangle(0u, 0u, 127u, 12u, White);
	draw_centered_str("SELECT SONG", Font_6x8, 6u, 3u);
	ssd1306_FillCircle(8u, 6u, 4u, Black);
	ssd1306_FillCircle(8u, 6u, 2u, White);

	ssd1306_SetCursor(0u, 0u);
	ssd1306_WriteString(" ", Font_6x8, Black);

	int count = (int) song_count;
	draw_list(song_list, count, music_selected, music_scroll, 14u, 114u, 121u);
}

/* Now Playing (DF): Header, scrolling title, animation, and compact volume */
void UI_DrawPlayDisplay_DF(void) {
	char buf[32];

	ssd1306_FillRectangle(0u, 0u, 127u, 12u, White);

	/* Play/Pause UI element */
	if (np_is_playing) {
		for (int i = 0; i < 5; i++)
			ssd1306_Line((uint8_t) (3 + i), (uint8_t) (2 + i),
					(uint8_t) (3 + i), (uint8_t) (10 - i), Black);
	} else {
		ssd1306_Line(3u, 2u, 3u, 10u, Black);
		ssd1306_Line(7u, 2u, 7u, 10u, Black);
	}

	snprintf(buf, sizeof(buf), "TRACK  %02d", (int) np_song_index + 1);
	ssd1306_SetCursor(32u, 3u);
	ssd1306_WriteString(buf, Font_6x8, Black);

	/* Scrolling marquee logic for long song titles */
	{
		int name_len = (int) strlen(np_song_name);
		int max_ch = 17;

		if (name_len <= max_ch) {
			draw_centered_str(np_song_name, Font_7x10, 7u, 18u);
		} else {
			if ((anim_tick - song_scroll_last) >= 5u) {
				song_scroll_last = anim_tick;
				if (song_scroll_pause > 0) {
					song_scroll_pause--;
				} else {
					song_scroll_offset++;
					if (song_scroll_offset >= name_len) {
						song_scroll_offset = 0;
						song_scroll_pause = 8;
					}
				}
			}
			char window[20];
			for (int i = 0; i < max_ch; i++)
				window[i] = np_song_name[(song_scroll_offset + i) % name_len];
			window[max_ch] = '\0';
			ssd1306_SetCursor(0u, 18u);
			ssd1306_WriteString(window, Font_7x10, White);
		}
	}

	ssd1306_Line(0u, 30u, 127u, 30u, White);
	draw_anim_scene(31u, 88u);

	ssd1306_Line(0u, 119u, 127u, 119u, White);
	ssd1306_SetCursor(2u, 121u);
	ssd1306_WriteString("V", Font_6x8, White);
	draw_vol_bar(12u, 120u, 113u, 7u, ui_data.volume);
}

/* Now Playing (Bluetooth): Modifies DF layout to include BT icon stamp */
void UI_DrawPlayDisplay_ble(void) {
	UI_DrawPlayDisplay_DF();
	draw_bt_icon(115, 1, Black);
}

/* Time Setup: HH:MM editor with blinking cursor and instructions */
void UI_DrawTimeSetup(void) {
	char buf[16];

	ssd1306_FillRectangle(0u, 0u, 127u, 13u, White);
	draw_centered_str("SET TIME", Font_6x8, 6u, 3u);

	snprintf(buf, sizeof(buf), "%02d", (int) time_h);
	ssd1306_SetCursor(22u, 20u);
	ssd1306_WriteString(buf, Font_16x26, White);

	ssd1306_SetCursor(55u, 20u);
	ssd1306_WriteString(":", Font_16x26, White);

	snprintf(buf, sizeof(buf), "%02d", (int) time_m);
	ssd1306_SetCursor(70u, 20u);
	ssd1306_WriteString(buf, Font_16x26, White);

	/* Blinking cursor effect over active field */
	if ((anim_tick % 10u) < 7u) {
		if (time_field == 0u)
			ssd1306_Line(22u, 49u, 52u, 49u, White);
		else
			ssd1306_Line(70u, 49u, 100u, 49u, White);
	}

	draw_centered_str("24H FORMAT", Font_6x8, 6u, 53u);
	ssd1306_Line(0u, 63u, 127u, 63u, White);

	ssd1306_SetCursor(4u, 67u);
	ssd1306_WriteString("UP / DN  : adjust", Font_6x8, White);
	ssd1306_SetCursor(4u, 78u);
	ssd1306_WriteString("OK       : next", Font_6x8, White);
	ssd1306_SetCursor(4u, 89u);
	ssd1306_WriteString("Hold OK  : confirm", Font_6x8, White);

	ssd1306_Line(0u, 100u, 127u, 100u, White);

	if (time_field == 0u) {
		ssd1306_FillCircle(52u, 115u, 5u, White);
		ssd1306_DrawCircle(76u, 115u, 5u, White);
	} else {
		ssd1306_DrawCircle(52u, 115u, 5u, White);
		ssd1306_FillCircle(76u, 115u, 5u, White);
	}
	ssd1306_SetCursor(38u, 122u);
	ssd1306_WriteString("H         M", Font_6x8, White);
}

/* ────────────────────────────────────────────────────────────────
 *  ALARM UI SCREENS
 * ────────────────────────────────────────────────────────────────*/
void UI_DrawTimeSubMenu(void) {
	static const char *submenu_items[2] = { " Set Time", " Alarms" };

	ssd1306_FillRectangle(0u, 0u, 127u, 12u, White);
	draw_centered_str("TIME MENU", Font_6x8, 6u, 3u);
	draw_list(submenu_items, 2, time_submenu_selected, 0, 20u, 100u, 127u);
}

void UI_DrawAlarmsList(void) {
	char list_buffers[10][16];
	const char *list_ptrs[10];

	/* Dynamically build text array based on active backend alarm status */
	for (int i = 0; i < backend_max_alarms; i++) {
		if (backend_alarms && backend_alarms[i].is_active) {
			snprintf(list_buffers[i], sizeof(list_buffers[i]), " %d. %02d:%02d",
					i + 1, backend_alarms[i].hour, backend_alarms[i].minute);
		} else {
			snprintf(list_buffers[i], sizeof(list_buffers[i]), " %d. [Empty]",
					i + 1);
		}
		list_ptrs[i] = list_buffers[i];
	}

	ssd1306_FillRectangle(0u, 0u, 127u, 12u, White);
	draw_centered_str("ALARMS", Font_6x8, 6u, 3u);
	draw_list(list_ptrs, backend_max_alarms, alarms_selected, alarms_scroll,
			14u, 114u, 121u);
}

void UI_DrawAlarmSetup(void) {
	char buf[16];

	ssd1306_FillRectangle(0u, 0u, 127u, 13u, White);
	draw_centered_str("ADD ALARM", Font_6x8, 6u, 3u);

	snprintf(buf, sizeof(buf), "%02d", (int) alarm_edit_h);
	ssd1306_SetCursor(22u, 30u);
	ssd1306_WriteString(buf, Font_16x26, White);

	ssd1306_SetCursor(55u, 30u);
	ssd1306_WriteString(":", Font_16x26, White);

	snprintf(buf, sizeof(buf), "%02d", (int) alarm_edit_m);
	ssd1306_SetCursor(70u, 30u);
	ssd1306_WriteString(buf, Font_16x26, White);

	if ((anim_tick % 10u) < 7u) {
		if (alarm_edit_field == 0u)
			ssd1306_Line(22u, 59u, 52u, 59u, White);
		else
			ssd1306_Line(70u, 59u, 100u, 59u, White);
	}

	ssd1306_Line(0u, 75u, 127u, 75u, White);
	ssd1306_SetCursor(4u, 82u);
	ssd1306_WriteString("UP/DN : adjust", Font_6x8, White);
	ssd1306_SetCursor(4u, 95u);
	ssd1306_WriteString("OK    : next", Font_6x8, White);
	ssd1306_SetCursor(4u, 108u);
	ssd1306_WriteString("HOLD  : save", Font_6x8, White);
}

/* ═══════════════════════════════════════════════════════════════
 *  OVERLAYS
 *  Note: Handle overlay timeouts in main/FreeRTOS task logic.
 * ═══════════════════════════════════════════════════════════════ */

void UI_DrawVolumeUp(void) {
	draw_vol_overlay_body(+1);
}

void UI_DrawVolumeDwn(void) {
	draw_vol_overlay_body(-1);
}

void UI_DrawLightsOverlay(void) {
	static const char *light_items[6] = { "Off", " Moonlight", " Starry Night",
			" Warm Breathing", " Colour Cycle", " Lamp", };

	ssd1306_FillRectangle(8u, 8u, 119u, 119u, Black);
	ssd1306_DrawRectangle(8u, 8u, 119u, 119u, White);

	ssd1306_FillRectangle(8u, 8u, 119u, 21u, White);
	draw_centered_str("LIGHTS", Font_6x8, 6u, 12u);
	draw_small_star(20, 15, Black);

	draw_list(light_items, 6, light_selected, 0, 23u, 82u, 114u);

	ssd1306_Line(8u, 106u, 119u, 106u, White);
	ssd1306_SetCursor(12u, 110u);
	ssd1306_WriteString("OK:apply  X:close", Font_6x8, White);
}

void UI_DrawTimerOverlay(void) {
	static const char *timer_items[6] = { "  OFF ", "   5 minutes",
			"  10 minutes", "  15 minutes", "  30 minutes", "  60 minutes", };

	ssd1306_FillRectangle(8u, 8u, 119u, 119u, Black);
	ssd1306_DrawRectangle(8u, 8u, 119u, 119u, White);

	ssd1306_FillRectangle(8u, 8u, 119u, 21u, White);
	draw_centered_str("SLEEP TIMER", Font_6x8, 6u, 12u);

	draw_list(timer_items, 6, timer_selected, 0, 23u, 82u, 114u);

	ssd1306_Line(8u, 106u, 119u, 106u, White);
	ssd1306_SetCursor(12u, 110u);
	ssd1306_WriteString("OK:set    X:close", Font_6x8, White);
}

void UI_DrawAlarmDeleteOverlay(void) {
	ssd1306_FillRectangle(18u, 30u, 109u, 85u, Black);
	ssd1306_DrawRectangle(18u, 30u, 109u, 85u, White);

	draw_centered_str("DELETE ALARM?", Font_6x8, 6u, 38u);
	ssd1306_Line(18u, 50u, 109u, 50u, White);

	if (alarm_delete_choice == 0) {
		ssd1306_FillRectangle(30u, 60u, 55u, 72u, White);
		ssd1306_SetCursor(34u, 63u);
		ssd1306_WriteString("YES", Font_6x8, Black);
	} else {
		ssd1306_SetCursor(34u, 63u);
		ssd1306_WriteString("YES", Font_6x8, White);
	}

	if (alarm_delete_choice == 1) {
		ssd1306_FillRectangle(70u, 60u, 90u, 72u, White);
		ssd1306_SetCursor(74u, 63u);
		ssd1306_WriteString("NO", Font_6x8, Black);
	} else {
		ssd1306_SetCursor(74u, 63u);
		ssd1306_WriteString("NO", Font_6x8, White);
	}
}

/* ═══════════════════════════════════════════════════════════════
 *  MAIN ENTRY POINT
 * ═══════════════════════════════════════════════════════════════ */
void ui_renderer_update(ui_state_t current_state, overlay_t *current_overlay) {

	ssd1306_Fill(Black);

	switch (current_state) {
	case UI_STATE_MAIN:
		UI_DrawMainScreen();
		break;
	case UI_STATE_MENU:
		UI_DrawMenu();
		break;
	case UI_STATE_MUSIC_LIST:
		UI_DrawMusicList();
		break;
	case UI_STATE_NOWPLAYING_DF:
		UI_DrawPlayDisplay_DF();
		break;
	case UI_STATE_NOWPLAYING_BLE:
		UI_DrawPlayDisplay_ble();
		break;
	case UI_STATE_TIME_SETUP:
		UI_DrawTimeSetup();
		break;
	case UI_STATE_TIME_SUBMENU:
		UI_DrawTimeSubMenu();
		break;
	case UI_STATE_ALARMS_LIST:
		UI_DrawAlarmsList();
		break;
	case UI_STATE_ALARM_SETUP:
		UI_DrawAlarmSetup();
		break;
	default:
		break;
	}

	if (current_overlay != NULL) {
		switch (current_overlay->type) {
		case OVERLAY_VOLUME_UP:
			UI_DrawVolumeUp();
			break;
		case OVERLAY_VOLUME_DOWN:
			UI_DrawVolumeDwn();
			break;
		case OVERLAY_LIGHT_MENU:
			UI_DrawLightsOverlay();
			break;
		case OVERLAY_TIMER:
			UI_DrawTimerOverlay();
			break;
		case OVERLAY_ALARM_DELETE:
			UI_DrawAlarmDeleteOverlay();
			break;
		case OVERLAY_ALARM_FIRING:
		    UI_DrawAlarmFiringOverlay();
		    break;
		default:
			// Overlay_None
			break;
		}
	}

	ssd1306_UpdateScreen();

	/* Advance global animation frame counter */
	anim_tick = (anim_tick + 1u) % ANIM_TICK_MAX;
}
