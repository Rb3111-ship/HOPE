/*
 * ui_renderer.c
 *
 *  Created on: 21 Apr 2026
 *      Author: whp27
 */
#include "ui_renderer.h"
#include "ui_state.h"

/*
 * ui_renderer.c
 *
 *  Baby Lullaby Device — Full UI Renderer
 *  Display : 128×128 SH1107 Monochrome OLED (SSD1306-compatible driver)
 *  Target  : STM32F412RET6  (HAL / FreeRTOS)
 *
 * ═══════════════════════════════════════════════════════════════
 *  QUICK-START: HOW TO HOOK THIS UP
 * ═══════════════════════════════════════════════════════════════
 *
 *  1. RENDERING TASK
 *     Call ui_renderer_update() every 100 ms from a FreeRTOS task
 *     or SysTick callback. This drives both display refresh and
 *     animation at ~10 Hz.
 *
 *       // In your UI task:
 *       for (;;) {
 *           ui_renderer_update(app.ui_state, &app.overlay);
 *           osDelay(100);
 *       }
 *
 *  2. LIVE DATA  →  populate ui_data before every ui_renderer_update() call
 *
 *       // RTC (STM32 HAL example):
 *       RTC_TimeTypeDef t;
 *       HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN);
 *       ui_data.hours   = t.Hours;
 *       ui_data.minutes = t.Minutes;
 *
 *       // DHT11 (call your own read function):
 *       DHT11_Read(&ui_data.temperature, &ui_data.humidity);
 *
 *       // DFPlayer volume:
 *       ui_data.volume = dfplayer_get_volume();   // range 0-30
 *
 *       // Now-playing track (DFPlayer):
 *       ui_data.song_index      = dfplayer_current_track() - 1; // 0-based
 *       ui_data.is_playing      = dfplayer_is_playing();
 *       ui_data.track_elapsed_s = dfplayer_elapsed_seconds();   // if available
 *       ui_data.track_total_s   = dfplayer_total_seconds();     // if available
 *       strncpy(ui_data.song_name, song_list[ui_data.song_index], 31);
 *
 *  3. BUTTON WIRING  →  call the navigation helpers from your button handler
 *       UP / DOWN  →  ui_song_list_navigate(±1) / ui_time_setup_adjust(±1)
 *       OK         →  ui_time_setup_next_field()
 *       MENU       →  ui_menu_navigate(±1)
 *       LIGHTS     →  ui_light_navigate(±1)
 *       TIMER      →  ui_timer_navigate(±1)
 *
 *  4. TIME CONFIRM FLOW  (UI_STATE_TIME_SETUP)
 *       - UP/DOWN     → ui_time_setup_adjust(±1)   increments active field
 *       - SHORT OK    → ui_time_setup_next_field()  hours ↔ minutes
 *       - LONG  OK    → uint8_t h, m;
 *                        ui_time_setup_get(&h, &m);
 *                        // write h,m to your RTC here
 *
 *  5. SONG LIST (UI_STATE_MUSIC_LIST)
 *       Edit song_list[] below. Set song_count = number of entries.
 *       On "select" button: read ui_data.song_index for the chosen track,
 *       then call dfplayer_play(ui_data.song_index + 1).
 *
 *
 *
 *
 */

#include "ui_renderer.h"
#include "ui_state.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* ═══════════════════════════════════════════════════════════════
 *  CONSTANTS
 * ═══════════════════════════════════════════════════════════════ */

#define DISPLAY_W       128u
#define DISPLAY_H       128u
#define VOLUME_MAX      30          /* DFPlayer Mini maximum volume            */
#define LIST_ROW_H      11          /* pixels per list row (Font_6x8 + 3 gap) */
#define LIST_VISIBLE    10          /* rows visible at once in a list          */

/*
 * ANIM_TICK_MAX controls how many ticks before the counter wraps.
 * At 10 Hz (100 ms per frame) → 240 ticks = 24-second full animation loop.
 * Stars, notes and the bunny have independent sub-periods so they never
 * all reset at the same time and the scene feels alive.
 */
#define ANIM_TICK_MAX   240u
const uint8_t song_count = 25;
/* ═══════════════════════════════════════════════════════════════
 *  LIVE DATA — populate before every ui_renderer_update() call
 * ═══════════════════════════════════════════════════════════════ */
void live_data_fill() {

	uint32_t last_sensor_read = 0;
	uint32_t now = osKernelGetTickCount();
	if ((now - last_sensor_read) >= pdMS_TO_TICKS(1000)) {
		last_sensor_read = now;

		get_Time(&ui_data.hours, &ui_data.minutes);
	}

	if ((now - last_sensor_read) >= pdMS_TO_TICKS(5000)) {
		last_sensor_read = now;
		ui_data.temperature = get_Temp_Data();
		ui_data.humidity = get_Hum_Data();
	}

	ui_data.volume = getVolume();

}

/* ═══════════════════════════════════════════════════════════════
 *  PRIVATE STATE
 * ═══════════════════════════════════════════════════════════════ */

static uint32_t anim_tick = 0; /* animation frame counter           */

/* Song list navigation */
static int music_scroll = 0; /* index of top visible song         */
static int music_selected = 0; /* index of highlighted song         */

/* Main menu icon selector (0=Music, 1=Bluetooth, 2=Time) */
static int menu_icon = 0;

/* Overlay list selections */
static int light_selected = 0; /* 0-4                               */
static int timer_selected = 0; /* 0-4                               */

/* Time setup editor */
static uint8_t time_field = 0; /* 0 = hours field, 1 = minutes      */
static uint8_t time_h = 0;
static uint8_t time_m = 0;

/* Song-name marquee scroll state (for long track names on now-playing screen) */
static int song_scroll_offset = 0;
static int song_scroll_pause = 0;
static uint32_t song_scroll_last = 0;

/* ── Now-playing internal cache ──────────────────────────────────────────
 *  Written once when the user confirms a song from the music list.
 *  Read every frame by UI_DrawPlayDisplay_DF / _ble.
 *  Never needs to talk to the music task.
 * ────────────────────────────────────────────────────────────────────── */
static char np_song_name[32] = "---";
static uint8_t np_song_index = 0;
static uint8_t np_is_playing = 0;

/* ═══════════════════════════════════════════════════════════════
 *  NAVIGATION HELPERS  (call from button handler)
 * ═══════════════════════════════════════════════════════════════ */

/* Song list: dir = +1 (down) or -1 (up) */
void ui_song_list_navigate(int8_t dir) {
	int count = (song_count > 0) ? (int) song_count : 1;
	music_selected += dir;
	if (music_selected < 0)
		music_selected = 0;
	if (music_selected >= count)
		music_selected = count - 1;
	/* Scroll window follows selection */
	if (music_selected < music_scroll)
		music_scroll = music_selected;
	if (music_selected >= music_scroll + LIST_VISIBLE)
		music_scroll = music_selected - LIST_VISIBLE + 1;
}

/* Main menu: cycles 0→1→2→0 (and reverse) */
void ui_menu_navigate(int8_t dir) {
	menu_icon = (menu_icon + (int) dir + 3) % 3;
}

/* Time setup: toggle between editing hours (field 0) and minutes (field 1) */
void ui_time_setup_next_field(void) {
	time_field ^= 1u;
}

/* Time setup: adjust the active field by dir (+1 or -1) */
void ui_time_setup_adjust(int8_t dir) {
	if (time_field == 0)
		time_h = (uint8_t) ((time_h + (int) dir + 24) % 24);
	else
		time_m = (uint8_t) ((time_m + (int) dir + 60) % 60);
}

/* Pre-load the time editor with the current RTC time.
 * Call this BEFORE transitioning to UI_STATE_TIME_SETUP:
 *   ui_time_setup_seed(rtc_hours, rtc_minutes); */
void ui_time_setup_seed() {
	time_h = ui_data.hours % 24u;
	time_m = ui_data.minutes % 60u;
	time_field = 0;
}

/* Read the confirmed time values. Call after the user presses "confirm". */
void ui_time_setup_get() {
	set_TimeMins(time_m);
	set_TimeH(time_h);
	confirm_time();
}

/* Lights overlay navigation */
void ui_light_navigate(int8_t dir) {

	light_selected = (light_selected + (int) dir + 6) % 6;
}

/* Timer overlay navigation */
void ui_timer_navigate(int8_t dir) {
	timer_selected = (timer_selected + (int) dir + 6) % 6;
}

/* Read selected light mode: 0=Moonlight 1=Starry Night 2=Warm Breathing
 *                            3=Color Cycle 4=Torch
 * Call after user confirms the lights overlay selection. */
int ui_get_light_mode(void) {
	return light_selected;
}

/* Read selected timer in minutes: 5 / 10 / 15 / 30 / 60.
 * Call after user confirms the timer overlay selection. */
int ui_get_timer_minutes(void) {
	static const int timers[6] = { 0, 5, 10, 15, 30, 60 };
	return timers[timer_selected];
}

/* Call this ONCE when the user selects a song and your state machine
 * transitions to UI_STATE_NOWPLAYING_DF.
 * Typically called in the same place you build the queue message
 * to the music task.
 *
 * Example in your button handler:
 *
 *   music_cmd_t cmd = { .track = music_selected + 1 };
 *   osMessageQueuePut(musicQHandle, &cmd, 0, 0);      // tell music task
 *   ui_nowplaying_set(music_selected,                  // tell UI renderer
 *                     song_list[music_selected]);
 *   app.ui_state = UI_STATE_NOWPLAYING_DF;
 */
void ui_nowplaying_set(uint8_t index, const char *name) {
	np_song_index = index;
	strncpy(np_song_name, name, sizeof(np_song_name) - 1);
	np_song_name[sizeof(np_song_name) - 1] = '\0';
	np_is_playing = 1;
	/* Reset marquee scroll so the new song name starts from the beginning */
	song_scroll_offset = 0;
	song_scroll_pause = 0;
	song_scroll_last = anim_tick;
}

/* Call this when your pause/resume button toggles playback.
 * You are already sending a command to the music task at that point,
 * so just call this alongside it.
 *
 * Example:
 *   music_cmd_t cmd = { .action = MUSIC_TOGGLE_PAUSE };
 *   osMessageQueuePut(musicQHandle, &cmd, 0, 0);
 *   ui_nowplaying_toggle_pause();
 */
void ui_nowplaying_toggle_pause(void) {
	np_is_playing ^= 1u;
}

// ui_renderer.c
uint8_t ui_get_selected_index(void) {
	return (uint8_t) music_selected;
}

/* Call from your button handler ONLY when in UI_STATE_NOWPLAYING_DF.
 * dir = +1 for next song, -1 for previous.
 * Wraps around at both ends of the list.
 *
 * Example in your button handler:
 *   if (app.ui_state == UI_STATE_NOWPLAYING_DF) {
 *       ui_nowplaying_skip(+1);                          // update UI
 *       music_cmd_t cmd = { .track = np_song_index + 1 };
 *       osMessageQueuePut(musicQHandle, &cmd, 0, 0);    // tell music task
 *   }
 *
 * Read the new track index AFTER calling this function since
 * np_song_index will have already been updated inside ui_nowplaying_set.
 */
void ui_nowplaying_skip(int8_t dir) {
	int count = (song_count > 0) ? (int) song_count : 1;
	int next = ((int) np_song_index + (int) dir + count) % count;

	/* Keep the list cursor in sync so if the user goes back to the
	 * list screen the highlight is on the correct song               */
	music_selected = next;
	if (music_selected < music_scroll)
		music_scroll = music_selected;
	if (music_selected >= music_scroll + LIST_VISIBLE)
		music_scroll = music_selected - LIST_VISIBLE + 1;

	ui_nowplaying_set((uint8_t) next, song_list[next]);
}
/* ═══════════════════════════════════════════════════════════════
 *  PRIVATE DRAWING HELPERS
 * ═══════════════════════════════════════════════════════════════ */

/* ── Coordinate clamp helper ── */
static inline uint8_t clamp8(int v) {
	if (v < 0)
		return 0u;
	if (v > 127)
		return 127u;
	return (uint8_t) v;
}

/* ── 5-pointed star polyline (pre-computed, R=8 r=4, centred on 0,0)
 *    Vertices 0-9 form the star; vertex 10 closes back to vertex 0.         */
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

/* ── Small star (R=4 r=2), lighter decoration ── */
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

/* ── Single music note ♪  at pixel (x,y) ── */
static void draw_music_note(int x, int y, SSD1306_COLOR col) {
	if (y < 0 || y > 119 || x < 0 || x > 117)
		return;
	ssd1306_Line(clamp8(x + 5), clamp8(y), clamp8(x + 5), clamp8(y + 8), col); /* stem */
	ssd1306_FillCircle(clamp8(x + 3), clamp8(y + 8), 2, col); /* head */
	ssd1306_Line(clamp8(x + 5), clamp8(y), clamp8(x + 9), clamp8(y + 3), col); /* flag */
}

/* ── Cute dancing bunny
 *    cx        = horizontal centre of the bunny
 *    head_top_y= pixel y of the TOP of the ears
 *    frame     = 0-3 animation cycle
 *
 *  Anatomy (relative to head_top_y):
 *    +0  – +10  : ears (tapered lines)
 *    +14         : head centre  (FillCircle r=6)
 *    +28         : body centre  (FillCircle r=8)
 *    arms swing ±3 px vertically based on frame
 *    legs alternate hop ±3 px                                                */
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
	ssd1306_DrawPixel(clamp8(cx - 2), clamp8(hcy - 1), Black); /* left eye  */
	ssd1306_DrawPixel(clamp8(cx + 2), clamp8(hcy - 1), Black); /* right eye */
	ssd1306_DrawPixel(clamp8(cx), clamp8(hcy + 1), Black); /* nose      */

	/* Body */
	int bcy = hcy + 14;
	ssd1306_FillCircle(clamp8(cx), clamp8(bcy), 8, White);

	/* Arms — swing up on frames 2-3, down on 0-1 */
	int arm_dy = (frame >= 2) ? -3 : 2;
	ssd1306_Line(clamp8(cx - 8), clamp8(bcy - 2), clamp8(cx - 14),
			clamp8(bcy - 2 + arm_dy), White);
	ssd1306_Line(clamp8(cx + 8), clamp8(bcy - 2), clamp8(cx + 14),
			clamp8(bcy - 2 - arm_dy), White);

	/* Legs — alternate hop (left on frames 1,3; right on frames 0,2) */
	int ll = ((frame == 1) || (frame == 3)) ? 3 : 0;
	int rl = ((frame == 0) || (frame == 2)) ? 3 : 0;
	int ly = bcy + 7;
	ssd1306_Line(clamp8(cx - 3), clamp8(ly), clamp8(cx - 5),
			clamp8(ly + 6 + ll), White);
	ssd1306_Line(clamp8(cx + 3), clamp8(ly), clamp8(cx + 5),
			clamp8(ly + 6 + rl), White);
	/* Feet (horizontal paw marks) */
	ssd1306_Line(clamp8(cx - 5), clamp8(ly + 6 + ll), clamp8(cx - 9),
			clamp8(ly + 6 + ll), White);
	ssd1306_Line(clamp8(cx + 5), clamp8(ly + 6 + rl), clamp8(cx + 9),
			clamp8(ly + 6 + rl), White);
}

/* ── Small Bluetooth icon, top-left anchor (x,y), ~10×12 px ── */
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

/* ── Large Bluetooth icon for the main menu, centred at (cx,cy), ~14×34 px ── */
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

/* ── Large music note for the main menu, anchor top-left at (x,y) ── */
static void draw_large_music_note(int x, int y) {
	/* Stem */
	ssd1306_Line(clamp8(x + 10), clamp8(y), clamp8(x + 10), clamp8(y + 22),
			White);
	/* Note head — ring (outer filled, inner black = hollow look) */
	ssd1306_FillCircle(clamp8(x + 6), clamp8(y + 22), 7, White);
	ssd1306_FillCircle(clamp8(x + 6), clamp8(y + 22), 3, Black);
	/* Flag */
	ssd1306_Line(clamp8(x + 10), clamp8(y), clamp8(x + 22), clamp8(y + 8),
			White);
	ssd1306_Line(clamp8(x + 22), clamp8(y + 8), clamp8(x + 10), clamp8(y + 13),
			White);
}

/* ── Clock-face icon for menu, centred at (cx,cy) with outer radius r ── */
static void draw_clock_icon(int cx, int cy, int r) {
	ssd1306_DrawCircle(clamp8(cx), clamp8(cy), (uint8_t) r, White);
	/* Minute hand — pointing nearly straight up (aesthetic) */
	ssd1306_Line(clamp8(cx), clamp8(cy), clamp8(cx - 4), clamp8(cy - r + 3),
			White);
	/* Hour hand — pointing upper-right (~2 o'clock) */
	ssd1306_Line(clamp8(cx), clamp8(cy), clamp8(cx + r / 2), clamp8(cy - r / 2),
			White);
	/* Centre dot */
	ssd1306_FillCircle(clamp8(cx), clamp8(cy), 2, White);
	/* Tick marks at 12/3/6/9 o'clock */
	ssd1306_Line(clamp8(cx), clamp8(cy - r), clamp8(cx), clamp8(cy - r + 3),
			White);
	ssd1306_Line(clamp8(cx + r), clamp8(cy), clamp8(cx + r - 3), clamp8(cy),
			White);
	ssd1306_Line(clamp8(cx), clamp8(cy + r), clamp8(cx), clamp8(cy + r - 3),
			White);
	ssd1306_Line(clamp8(cx - r), clamp8(cy), clamp8(cx - r + 3), clamp8(cy),
			White);
}

/* ── Horizontal volume bar ──
 *    x1,y1  = top-left corner
 *    width  = total width (pixels)
 *    height = total height (pixels)
 *    vol    = current volume 0-VOLUME_MAX                                     */
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

/* ── Vertical scrollbar on the right edge of a list ──
 *    x,y      = top-left of the scrollbar track
 *    track_h  = total track height in pixels
 *    total    = total item count
 *    visible  = number of visible rows
 *    top      = current top-of-window item index                              */
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

/* ── Centre a string horizontally on the display
 *    char_w = approximate character width of the chosen font:
 *             Font_6x8 → 6,  Font_7x10 → 7,  Font_11x18 → 11,  Font_16x26 → 16   */
static void draw_centered_str(const char *str, SSD1306_Font_t font,
		uint8_t char_w, uint8_t y) {
	uint8_t len = (uint8_t) strlen(str);
	uint8_t total_w = len * char_w;
	uint8_t x = (total_w < DISPLAY_W) ? (DISPLAY_W - total_w) / 2u : 0u;
	ssd1306_SetCursor(x, y);
	ssd1306_WriteString((char*) str, font, White);
}

/* ── Generic scrollable list renderer
 *    Used for songs, light modes, and timer options.
 *    items[]   = array of string pointers
 *    count     = number of items
 *    selected  = currently highlighted item index
 *    top       = index of the topmost visible item
 *    list_y    = top pixel of the list area
 *    list_h    = height of the list area in pixels
 *    list_w    = width of the list area (scrollbar sits to the right)         */
static void draw_list(const char **items, int count, int selected, int top,
		uint8_t list_y, uint8_t list_h, uint8_t list_w) {
	int visible = list_h / LIST_ROW_H;
	for (int i = 0; i < visible; i++) {
		int idx = top + i;
		if (idx >= count)
			break;
		uint8_t row_y = list_y + (uint8_t) (i * LIST_ROW_H);
		if (idx == selected) {
			/* Highlighted row: white fill, black text */
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

/* ── Entire animated scene: bunny + floating stars + drifting music note
 *
 *    zone_y = pixel y where the animation zone begins
 *    zone_h = pixel height of the animation zone
 *
 *  The bunny is placed on the left side of the zone and hops in a sine-like
 *  curve approximated by hop[]. Stars drift upward in staggered columns on
 *  the right. A music note drifts up near the centre.
 *
 *  Animation speed is governed by anim_tick.
 *  Tune the divisor in anim_step / draw_frm to speed up or slow down.        */
static void draw_anim_scene(uint8_t zone_y, uint8_t zone_h) {
	/* Hop profile over 8 steps (pixels, negative = upward).
	 * Steps 0-6: jump arc. Step 7: ground contact pause.                     */
	static const int8_t hop[8] = { 0, -3, -6, -8, -6, -3, 0, 0 };

	uint8_t anim_step = (uint8_t) ((anim_tick / 6u) % 8u); /* 0.6 s per step  */
	uint8_t draw_frm = (uint8_t) ((anim_tick / 6u) % 4u); /* 4-frame cycle   */

	/* Bunny: stand with feet 38 px above the bottom of the zone */
	int bunny_cx = 30;
	int bunny_ground = (int) zone_y + (int) zone_h - 38;
	int bunny_top_y = bunny_ground + hop[anim_step];
	draw_bunny(bunny_cx, bunny_top_y, draw_frm);

	/* Ground line */
	ssd1306_Line(4u, zone_y + zone_h - 2u, 123u, zone_y + zone_h - 2u, White);

	/* Floating stars — three columns with different periods and phases */
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

	/* Floating music note — slightly left of centre, different period */
	{
		uint8_t t = (uint8_t) (anim_tick % 38u);
		int ny = ((int) zone_y + (int) zone_h - 12)
				- (int) ((uint32_t) t * zone_h / 38u);
		if (ny >= (int) zone_y)
			draw_music_note(52, ny, White);
	}
}

/* ── Volume overlay body (shared between VOL_UP and VOL_DOWN)
 *    direction: +1 draws a plus sign, -1 draws a minus sign                  */
static void draw_vol_overlay_body(int direction) {
	/* Box */
	ssd1306_FillRectangle(18u, 30u, 109u, 117u, Black);
	ssd1306_DrawRectangle(18u, 30u, 109u, 117u, White);

	/* "VOLUME" title */
	draw_centered_str("VOLUME", Font_7x10, 7u, 33u);
	ssd1306_Line(18u, 44u, 109u, 44u, White);

	/* Speaker cone */
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
	/* Sound waves */
	ssd1306_DrawArc(38u, 64u, 9u, 300u, 120u, White);
	ssd1306_DrawArc(38u, 64u, 15u, 300u, 120u, White);

	/* +/- indicator circle */
	ssd1306_DrawCircle(85u, 64u, 13u, White);
	ssd1306_Line(79u, 64u, 91u, 64u, White); /* horizontal bar (both +/-) */
	if (direction > 0)
		ssd1306_Line(85u, 58u, 85u, 70u, White); /* vertical bar for + */

	/* Numeric volume value, centred */
	{
		char buf[8];
		snprintf(buf, sizeof(buf), "%d", (int) ui_data.volume);
		draw_centered_str(buf, Font_11x18, 11u, 82u);
	}

	/* Volume bar */
	draw_vol_bar(22u, 103u, 85u, 8u, ui_data.volume);
}

/* ═══════════════════════════════════════════════════════════════
 *  SCREEN IMPLEMENTATIONS
 * ═══════════════════════════════════════════════════════════════ */

/* ────────────────────────────────────────────────────────────────
 *  MAIN SCREEN
 *
 *  Layout (128×128):
 *    y  0-20   Large 24-h clock (Font_11x18, centred)
 *    y 21      Divider line
 *    y 22-32   Temperature and humidity (Font_6x8)
 *    y 33      Divider line
 *    y 34-105  Animation zone (bunny, stars, music note)
 *    y 106     Divider line
 *    y 107-127 Volume label + bar
 *
 *  Hook-up: set ui_data.hours, minutes, temperature, humidity, volume
 * ────────────────────────────────────────────────────────────────*/
void UI_DrawMainScreen(void) {
	char buf[32];

	/* Clock — Font_11x18: each char ~11 px wide, "HH:MM" = 5 chars = 55 px */
	snprintf(buf, sizeof(buf), "%02d:%02d", ui_data.hours, ui_data.minutes);
	ssd1306_SetCursor(36u, 2u);
	ssd1306_WriteString(buf, Font_11x18, White);

	ssd1306_Line(0u, 21u, 127u, 21u, White);

	/* Temperature and humidity on one line */
	snprintf(buf, sizeof(buf), " %dC    Hum:%d%%", (int) ui_data.temperature,
			(int) ui_data.humidity);
	ssd1306_SetCursor(2u, 24u);
	ssd1306_WriteString(buf, Font_6x8, White);

	ssd1306_Line(0u, 33u, 127u, 33u, White);

	/* Animation zone: y=34 to y=105, height = 72 px */
	draw_anim_scene(34u, 72u);

	ssd1306_Line(0u, 106u, 127u, 106u, White);

	/* Volume bar */
	ssd1306_SetCursor(2u, 110u);
	ssd1306_WriteString("VOL", Font_6x8, White);
	draw_vol_bar(26u, 110u, 98u, 8u, ui_data.volume);
}

/* ────────────────────────────────────────────────────────────────
 *  MENU SCREEN
 *
 *  One large icon at a time with navigation arrows and page dots.
 *  Icons: 0 = Music  1 = Bluetooth  2 = Time Setup
 *
 *  Hook-up: call ui_menu_navigate(+1/-1) from your NEXT/PREV buttons.
 *  On "select" button: read menu_icon via ui_get_menu_icon() or
 *  directly transition ui_state based on which icon is shown:
 *    0 → UI_STATE_MUSIC_LIST
 *    1 → UI_STATE_NOWPLAYING_BLE  (or your BLE pairing screen)
 *    2 → UI_STATE_TIME_SETUP
 *
 *  HOW TO ADD AN ICON:
 *    1. Change the modulus in ui_menu_navigate from 3 to your new count.
 *    2. Add a case in the switch below with your draw function.
 *    3. Add a label string to labels[].
 *    4. Add one more page dot in the dots loop.
 * ────────────────────────────────────────────────────────────────*/

/* Read which menu icon is currently shown (0=Music, 1=BT, 2=Time) */
int ui_get_menu_icon(void) {
	return menu_icon;
}

void UI_DrawMenu(void) {
	static const char *labels[3] = { "MUSIC", "BLUETOOTH", "TIME" };

	/* Left arrow (shown only if not first icon) */
	if (menu_icon > 0) {
		ssd1306_Line(7u, 64u, 13u, 58u, White);
		ssd1306_Line(7u, 64u, 13u, 70u, White);
		ssd1306_Line(7u, 64u, 13u, 64u, White);
	}
	/* Right arrow (shown only if not last icon) */
	if (menu_icon < 2) {
		ssd1306_Line(120u, 64u, 114u, 58u, White);
		ssd1306_Line(120u, 64u, 114u, 70u, White);
		ssd1306_Line(120u, 64u, 114u, 64u, White);
	}

	/* Icon, centred in the display */
	switch (menu_icon) {
	case 0:
		draw_large_music_note(42, 38);
		break; /* anchor top-left ~42,38 */
	case 1:
		draw_large_bt_icon(64, 62);
		break;
	case 2:
		draw_clock_icon(64, 60, 28);
		break;
	}

	/* Label */
	draw_centered_str(labels[menu_icon], Font_7x10, 7u, 106u);

	/* Page indicator dots */
	for (int i = 0; i < 3; i++) {
		uint8_t dx = (uint8_t) (55 + i * 9);
		if (i == menu_icon)
			ssd1306_FillCircle(dx, 121u, 3u, White);
		else
			ssd1306_DrawCircle(dx, 121u, 3u, White);
	}
}

/* ────────────────────────────────────────────────────────────────
 *  MUSIC LIST SCREEN
 *
 *  Scrollable list of up to 20 songs with highlight and scrollbar.
 *
 *  Hook-up:
 *    1. Populate song_list[] above with your track names.
 *    2. Set song_count = number of tracks.
 *    3. Call ui_song_list_navigate(+1/-1) from UP/DOWN buttons.
 * ────────────────────────────────────────────────────────────────*/
void UI_DrawMusicList(void) {
	/* Header bar */
	ssd1306_FillRectangle(0u, 0u, 127u, 12u, White);
	draw_centered_str("SELECT SONG", Font_6x8, 6u, 3u);
	/* Small note icon in header */
	ssd1306_FillCircle(8u, 6u, 4u, Black);
	ssd1306_FillCircle(8u, 6u, 2u, White);

	ssd1306_SetCursor(0u, 0u);
	ssd1306_WriteString(" ", Font_6x8, Black); /* force cursor reset if needed */

	int count = (int) song_count;
	draw_list(song_list, count, music_selected, music_scroll, 14u, 114u, 121u);
}

/* ────────────────────────────────────────────────────────────────
 *  NOW PLAYING — DFPlayer
 *
 *  Layout:
 *    y  0-12   Inverted header bar: play/pause icon + "TRACK XX"
 *    y 13-26   Song name (auto-scrolls if >17 chars at Font_7x10)
 *    y 27-35   Elapsed / total time text
 *    y 36-43   Progress bar
 *    y 44      Divider
 *    y 45-117  Animation zone (bunny + stars)
 *    y 118     Divider
 *    y 119-127 Compact volume bar
 *

 * ────────────────────────────────────────────────────────────────*/
void UI_DrawPlayDisplay_DF(void) {
	char buf[32];

	/* ── Header bar (inverted) ── */
	ssd1306_FillRectangle(0u, 0u, 127u, 12u, White);

	/* Play ▶ or pause ‖ icon in black on white header */
	if (np_is_playing) {
		for (int i = 0; i < 5; i++)
			ssd1306_Line((uint8_t) (3 + i), (uint8_t) (2 + i),
					(uint8_t) (3 + i), (uint8_t) (10 - i), Black);
	} else {
		ssd1306_Line(3u, 2u, 3u, 10u, Black);
		ssd1306_Line(7u, 2u, 7u, 10u, Black);
	}

	/* Track number */
	snprintf(buf, sizeof(buf), "TRACK  %02d", (int) np_song_index + 1);
	ssd1306_SetCursor(32u, 3u);
	ssd1306_WriteString(buf, Font_6x8, Black);

	/* ── Song name — scrolling marquee for long names ── */
	{
		int name_len = (int) strlen(np_song_name);
		int max_ch = 17; /* ~119 px at Font_7x10 (7 px/char) */

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

	/* ── Animation zone: full remaining space ── */
	/* y=31 to y=118, height = 88 px — more room now that the progress
	 * bar and time string are gone, so the bunny gets a bigger stage.  */
	draw_anim_scene(31u, 88u);

	/* ── Compact volume strip ── */
	ssd1306_Line(0u, 119u, 127u, 119u, White);
	ssd1306_SetCursor(2u, 121u);
	ssd1306_WriteString("V", Font_6x8, White);
	draw_vol_bar(12u, 120u, 113u, 7u, ui_data.volume);
}
/* ────────────────────────────────────────────────────────────────
 *  NOW PLAYING — Bluetooth
 *
 *  Same layout as DFPlayer screen, but with a small Bluetooth icon
 *  stamped into the header bar (replacing the track-number suffix).
 *
 *  Hook-up: same as UI_DrawPlayDisplay_DF.
 *  Populate ui_data.song_name from your BLE AVRCP metadata callback.
 *  If track total is unknown (common on BLE) set track_total_s = 0
 *  and the progress bar will stay empty (shows --:--).
 * ────────────────────────────────────────────────────────────────*/
void UI_DrawPlayDisplay_ble(void) {
	/* Draw the base DF layout (header, name, progress, animation, volume) */
	UI_DrawPlayDisplay_DF();

	/* Stamp BT icon into the header bar in BLACK (bar is white) */
	draw_bt_icon(115, 1, Black);
}

/* ────────────────────────────────────────────────────────────────
 *  TIME SETUP SCREEN
 *
 *  Layout:
 *    y  0-13   Title "SET TIME" with divider
 *    y 15-49   Large HH:MM display (Font_16x26)
 *              NOTE: Font_16x26 requires the ssd1306_fonts.h entry for it.
 *              If your build does not have Font_16x26, replace with Font_11x18
 *              and adjust the x offsets (chars ~11px wide each).
 *    y 50      Blinking underline on active field
 *    y 54      "24H FORMAT" label
 *    y 68-96   Key instructions
 *    y 110-127 Field indicator dots + border
 *
 *  Button mapping (implement in your button handler):
 *    Your "UP"   button → ui_time_setup_adjust(+1)
 *    Your "DOWN" button → ui_time_setup_adjust(-1)
 *    Short "OK"         → ui_time_setup_next_field()
 *    Long  "OK"         → ui_time_setup_get(&h, &m);
 *                          HAL_RTC_SetTime(...)  ← write to your RTC here
 *
 *  To pre-load with the current RTC time before entering this screen:
 *    ui_time_setup_seed(rtc_hours, rtc_minutes);
 * ────────────────────────────────────────────────────────────────*/
void UI_DrawTimeSetup(void) {
	char buf[16];

	/* Title */
	ssd1306_FillRectangle(0u, 0u, 127u, 13u, White);
	draw_centered_str("SET TIME", Font_6x8, 6u, 3u);

	/* Hours value */
	snprintf(buf, sizeof(buf), "%02d", (int) time_h);
	ssd1306_SetCursor(22u, 20u);
	ssd1306_WriteString(buf, Font_16x26, White);

	/* Colon */
	ssd1306_SetCursor(55u, 20u);
	ssd1306_WriteString(":", Font_16x26, White);

	/* Minutes value */
	snprintf(buf, sizeof(buf), "%02d", (int) time_m);
	ssd1306_SetCursor(70u, 20u);
	ssd1306_WriteString(buf, Font_16x26, White);

	/* Blinking underline on the active field
	 * On 7 ticks, off 3 ticks → 0.7 s on / 0.3 s off @10Hz                 */
	if ((anim_tick % 10u) < 7u) {
		if (time_field == 0u)
			ssd1306_Line(22u, 49u, 52u, 49u, White); /* hours underline   */
		else
			ssd1306_Line(70u, 49u, 100u, 49u, White); /* minutes underline */
	}

	/* Format label */
	draw_centered_str("24H FORMAT", Font_6x8, 6u, 53u);
	ssd1306_Line(0u, 63u, 127u, 63u, White);

	/* Instructions */
	ssd1306_SetCursor(4u, 67u);
	ssd1306_WriteString("UP / DN  : adjust", Font_6x8, White);
	ssd1306_SetCursor(4u, 78u);
	ssd1306_WriteString("OK       : next", Font_6x8, White);
	ssd1306_SetCursor(4u, 89u);
	ssd1306_WriteString("Hold OK  : confirm", Font_6x8, White);

	ssd1306_Line(0u, 100u, 127u, 100u, White);

	/* Field dots (hours / minutes indicator) */
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

/* ═══════════════════════════════════════════════════════════════
 *  OVERLAYS
 * ═══════════════════════════════════════════════════════════════
 *
 *  All overlays are drawn ON TOP of the current screen (Fill(Black)
 *  was called at the top of ui_renderer_update(), so you get a clean
 *  layer order: screen first, then overlay on top).
 *
 *  TIMEOUT MANAGEMENT (do this in your main/FreeRTOS task):
 *    if (overlay.active && overlay.timeout_ms > 0) {
 *        if (overlay.timeout_ms <= FRAME_PERIOD_MS)
 *            overlay.active = 0;
 *        else
 *            overlay.timeout_ms -= FRAME_PERIOD_MS;
 *    }
 *  Replace FRAME_PERIOD_MS with your ui task period (e.g. 100).
 * ═══════════════════════════════════════════════════════════════ */

/* ────────────────────────────────────────────────────────────────
 *  VOLUME UP OVERLAY
 *  timeout_ms: set to 1500 when user presses volume-up button.
 * ────────────────────────────────────────────────────────────────*/
void UI_DrawVolumeUp(void) {
	draw_vol_overlay_body(+1);
}

/* ────────────────────────────────────────────────────────────────
 *  VOLUME DOWN OVERLAY
 *  timeout_ms: set to 1500 when user presses volume-down button.
 * ────────────────────────────────────────────────────────────────*/
void UI_DrawVolumeDwn(void) {
	draw_vol_overlay_body(-1);
}

/* ────────────────────────────────────────────────────────────────
 *  LIGHTS OVERLAY
 *
 *  timeout_ms: set to 5000 (5 seconds). Dismissed by timeout OR by
 *  pressing your "lights" button a second time (caller sets active=0).
 *
 *  Navigate with: ui_light_navigate(+1/-1) from UP/DOWN buttons.
 *  Confirm with:  int mode = ui_get_light_mode();  then apply to LED ring.
 *  Light modes:   0=Moonlight  1=Starry Night  2=Warm Breathing
 *                 3=Color Cycle  4=Torch
 * ────────────────────────────────────────────────────────────────*/
void UI_DrawLightsOverlay(void) {

	static const char *light_items[6] = { "Off", " Moonlight", " Starry Night",
			" Warm Breathing", " Colour Cycle", " Lamp", };

	ssd1306_FillRectangle(8u, 8u, 119u, 119u, Black);
	ssd1306_DrawRectangle(8u, 8u, 119u, 119u, White);

	/* Title bar */
	ssd1306_FillRectangle(8u, 8u, 119u, 21u, White);
	draw_centered_str("LIGHTS", Font_6x8, 6u, 12u);
	/* Small star icon left of title */
	draw_small_star(20, 15, Black);

	draw_list(light_items, 6, light_selected, 0, 23u, 82u, 114u);

	/* Footer */
	ssd1306_Line(8u, 106u, 119u, 106u, White);
	ssd1306_SetCursor(12u, 110u);
	ssd1306_WriteString("OK:apply  X:close", Font_6x8, White);
}

/* ────────────────────────────────────────────────────────────────
 *  SLEEP TIMER OVERLAY
 *
 *  timeout_ms: set to 5000 (5 seconds). Dismissed by timeout or button.
 *
 *  Navigate with: ui_timer_navigate(+1/-1).
 *  Confirm with:  int mins = ui_get_timer_minutes();
 *                 then start your countdown timer.
 *  Options: 5 / 10 / 15 / 30 / 60 minutes.
 * ────────────────────────────────────────────────────────────────*/
void UI_DrawTimerOverlay(void) {
	static const char *timer_items[6] = { "  OFF ", "   5 minutes",
			"  10 minutes", "  15 minutes", "  30 minutes", "  60 minutes", };

	ssd1306_FillRectangle(8u, 8u, 119u, 119u, Black);
	ssd1306_DrawRectangle(8u, 8u, 119u, 119u, White);

	/* Title bar */
	ssd1306_FillRectangle(8u, 8u, 119u, 21u, White);
	draw_centered_str("SLEEP TIMER", Font_6x8, 6u, 12u);

	draw_list(timer_items, 6, timer_selected, 0, 23u, 82u, 114u);

	/* Footer */
	ssd1306_Line(8u, 106u, 119u, 106u, White);
	ssd1306_SetCursor(12u, 110u);
	ssd1306_WriteString("OK:set    X:close", Font_6x8, White);
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
	case UI_STATE_MUSIC_MENU:
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
	default:
		break;
	}

	if (current_overlay != NULL && current_overlay->active) {
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
		default:
			break;
		}
	}

	ssd1306_UpdateScreen();

	/* Advance animation counter */
	anim_tick = (anim_tick + 1u) % ANIM_TICK_MAX;
}

