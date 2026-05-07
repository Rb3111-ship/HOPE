/*
 * ui_task.c
 *
 *  Created on: 21 Apr 2026
 *      Author: whp27
 */

#include "tasks.h"
#include "app_queue.h"
#include "main.h"
#include "ui_state.h"
#include "music.h"
#include "ui_renderer.h"
#include <stdint.h>
#include "task.h"
#include "cmsis_os.h"
#include "alarm_service.h"

static ui_state_t currentState = UI_STATE_MAIN;
static ui_state_t previousState;
static overlay_t previousOverlay;
static overlay_t currentOverlay = { .type = OVERLAY_NONE };
static music_msg_t music_msg;
static uint8_t saved_song = 0; //default saved song is song 1
static uint32_t lightOverlay_open_tick;
static uint32_t volOverlay_open_tick;
static uint32_t timer_start;
static uint8_t play_state = 0;
static uint8_t set_Vol = 5;
static uint8_t vol_flag = 0;
static uint8_t timer_flag = 0;
volatile uint32_t timeout_ms = 0;

#define LIGHT_OVERLAY_PERIOD_MS 5000
#define VOL_OVERLAY_PERIOD_MS 1500
#define FIVE_MIN_PERIOD_MS 300000
#define TEN_MIN_PERIOD_MS 600000
#define FIFTEEN_MIN_PERIOD_MS 900000
#define THIRTY_MIN_PERIOD_MS 1800000
#define SIXTY_MIN_PERIOD_MS 3600000

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) { // called automatically by HAL when EXTI interrupt occurs
	BaseType_t xHigherPriorityTaskWoken = pdFALSE; //GPIO_Pin used to compare with pins used for touch sensors
	ui_msg_t msg;

	switch (GPIO_Pin) {
	case BTN_VOL_DWN_Pin:
		msg.evt = EVT_BTN_VOL_DOWN;
		break;
	case BTN_PLAY_Pin:
		msg.evt = EVT_BTN_PLAY;
		break;
	case BTN_UP_Pin:
		msg.evt = EVT_BTN_NEXT;
		break;
	case BTN_LIGHT_Pin:
		msg.evt = EVT_BTN_LIGHT;
		break;
	case BTN_DWN_Pin:
		msg.evt = EVT_BTN_PREV;
		break;
	case BTN_VOL_UP_Pin:
		msg.evt = EVT_BTN_VOL_UP;
		break;
	case BTN_TIMER_Pin:
		msg.evt = EVT_BTN_TIMER;
		break;
	case BTN_MUSIC_Pin:
		msg.evt = EVT_BTN_MENU;
		break;
	}

	xQueueSendFromISR(uiQueueHandle, &msg, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

}

void volume(evt_type_t msg) {

	vol_flag = 1;

	volOverlay_open_tick = osKernelGetTickCount();
	previousOverlay.type = currentOverlay.type;
	if (msg == EVT_BTN_VOL_UP) {
		currentOverlay.type = OVERLAY_VOLUME_UP;
		set_Vol++;
	} else if (msg == EVT_BTN_VOL_DOWN) {
		currentOverlay.type = OVERLAY_VOLUME_DOWN;
		set_Vol--;
	}

	if (set_Vol < 0)
		set_Vol = 0;
	if (set_Vol > 30)
		set_Vol = 30;
	set_volume(set_Vol);
	music_msg.comm = EVT_SET_VOL;
	music_msg.data = set_Vol;
	xQueueSend(musicQueueHandle, &music_msg, portMAX_DELAY);
}

void timer_counter(uint16_t timer_minutes) {
	timer_start = osKernelGetTickCount();
	switch (timer_minutes) {
	case 0:
		// turns off timers
		timer_flag = 0;
		break;
	case 5:
		timer_flag = 1;
		timeout_ms = FIVE_MIN_PERIOD_MS;
		break;
	case 10:
		timer_flag = 1;
		timeout_ms = TEN_MIN_PERIOD_MS;
		break;
	case 15:
		timer_flag = 1;
		timeout_ms = FIFTEEN_MIN_PERIOD_MS;
		break;
	case 30:
		timer_flag = 1;
		timeout_ms = THIRTY_MIN_PERIOD_MS;
		break;
	case 60:
		timer_flag = 1;
		timeout_ms = SIXTY_MIN_PERIOD_MS;
		break;

	}
}

void ui_Task(void *pvParameters) {

	ui_msg_t msg;
	const TickType_t xDelay100ms = pdMS_TO_TICKS(100UL);

	for (;;) {

		if (xQueueReceive(uiQueueHandle, &msg, xDelay100ms) == pdPASS) {

			switch (currentState) {

			case UI_STATE_MAIN:
				if (msg.evt == EVT_BTN_MENU) {
					currentState = UI_STATE_MENU;

				}

				else if (msg.evt == EVT_BTN_TIMER) {
					currentOverlay.type = OVERLAY_TIMER;
					currentState = UI_TIMER;

				}
				break;

			case UI_STATE_MENU:
				if (msg.evt == EVT_BTN_MENU) {
					currentState = UI_STATE_MAIN;
				}

				else if (msg.evt == EVT_BTN_NEXT) {
					//move down menu
					ui_menu_navigate(1);
				}

				else if (msg.evt == EVT_BTN_PREV) {
					// move up menu
					ui_menu_navigate(-1);
				}

				else if (msg.evt == EVT_BTN_PLAY) {

					switch (ui_get_menu_icon()) {
					case 0:
						currentState = UI_STATE_MUSIC_LIST;
						music_msg.data = 0;
						xQueueSend(musicQueueHandle, &music_msg, portMAX_DELAY);
						break;
					case 1:
						currentState = UI_STATE_NOWPLAYING_BLE;
						music_msg.comm = EVT_BLE_ON;
						music_msg.data = 0;
						xQueueSend(musicQueueHandle, &music_msg, portMAX_DELAY);

						break;
					case 2:
						currentState = UI_STATE_TIME_SUBMENU;
					}
				}

				break;

				//
				//
				//
				//
				//
				//
				//
				//
				//
				//
				//
				//
				//
				//
				//
				//
				//

			case UI_STATE_TIME_SUBMENU:
				if (msg.evt == EVT_BTN_MENU) {
					currentState = UI_STATE_MAIN;
				}

				else if (msg.evt == EVT_BTN_NEXT) {
					//move down menu
					ui_time_submenu_navigate(1);

				}

				else if (msg.evt == EVT_BTN_PREV) {
					// move up menu
					ui_time_submenu_navigate(-1);
				}

				else if (msg.evt == EVT_BTN_PLAY) {

					switch (ui_get_time_submenu_selection()) {
					case 0:
						currentState = UI_STATE_TIME_SETUP;
						ui_time_setup_seed();
						break;
					case 1:
						currentState = UI_STATE_ALARMS_LIST;
						break;

					}
				}
				break;

			case UI_STATE_ALARMS_LIST:
				SoftwareAlarm alarm;
				//Get alarm data from alarm and show it in the UI

				if (msg.evt == EVT_BTN_MENU) {
					currentState = UI_STATE_TIME_SUBMENU;
				}
				// navigate a list of alarms
				else if (msg.evt == EVT_BTN_NEXT) {
					//move down menu
					ui_alarms_list_navigate(1);

				}

				else if (msg.evt == EVT_BTN_PREV) {
					// move up menu
					ui_alarms_list_navigate(-1);
				}

				if (msg.evt == EVT_BTN_PLAY) {
					if (ui_is_selected_alarm_empty()) { //if the alarm is active 1
						currentOverlay.type = OVERLAY_ALARM_DELETE;
					}

					else {
						ui_alarm_setup_seed();
						currentState = UI_STATE_ALARM_SETUP;
					}
				}

				break;

			case UI_STATE_TIME_SETUP:
				if (msg.evt == EVT_BTN_MENU) {
					currentState = UI_STATE_TIME_SUBMENU;

				}

				if (msg.evt == EVT_BTN_PREV) {
					ui_time_setup_adjust(-1);
				}

				if (msg.evt == EVT_BTN_NEXT) {
					ui_time_setup_adjust(1);
				}

				if (msg.evt == EVT_BTN_PLAY) {
					ui_time_setup_next_field();
				}

				if (msg.evt == EVT_BTN_TIMER) {
					ui_time_setup_get();
					currentState = UI_STATE_TIME_SUBMENU;
				}

				break;

			case UI_STATE_ALARM_SETUP:
				if (msg.evt == EVT_BTN_MENU) {
					currentState = UI_STATE_ALARMS_LIST;

				}

				if (msg.evt == EVT_BTN_PREV) {
					ui_alarm_setup_adjust(-1);
				}

				if (msg.evt == EVT_BTN_NEXT) {
					ui_alarm_setup_adjust(1);
				}

				if (msg.evt == EVT_BTN_PLAY) {
					ui_alarm_setup_next_field();
				}

				if (msg.evt == EVT_BTN_TIMER) {
					ui_alarm_setup_confirm();
					currentState = UI_STATE_ALARMS_LIST;
				}

				break;

//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//
//

			case UI_STATE_MUSIC_LIST:
				if (msg.evt == EVT_BTN_MENU) {
					currentState = UI_STATE_MENU;
					music_msg.comm = EVT_STOP;
					music_msg.data = 0;
					xQueueSend(musicQueueHandle, &music_msg, portMAX_DELAY);

				} else if (msg.evt == EVT_BTN_NEXT) {
					// move down list
					ui_song_list_navigate(1);
				}

				else if (msg.evt == EVT_BTN_PREV) {
					//move up list
					ui_song_list_navigate(-1);
				}

				else if (msg.evt == EVT_BTN_PLAY) {

					uint8_t selected_song = ui_get_selected_index();
					music_msg.comm = EVT_PLAY;
					music_msg.data = selected_song + 1;
					xQueueSend(musicQueueHandle, &music_msg, portMAX_DELAY);
					ui_nowplaying_set(selected_song,     // tell UI renderer
							song_list[selected_song]);
					play_state = 1; // the song is playing  not paused
					currentState = UI_STATE_NOWPLAYING_DF;
				}

				else if (msg.evt == EVT_BTN_VOL_UP
						|| msg.evt == EVT_BTN_VOL_DOWN) {
					volume(msg.evt);

				}

				break;

			case UI_STATE_NOWPLAYING_DF:
				if (msg.evt == EVT_BTN_MENU) {
					currentState = UI_STATE_MUSIC_LIST;
				}

				else if (msg.evt == EVT_BTN_NEXT) {
					// TODO:move NEXT song
					ui_nowplaying_skip(+1);
					uint8_t selected_song = ui_get_selected_index();
					music_msg.comm = EVT_NEXT;
					music_msg.data = selected_song + 1;
					xQueueSend(musicQueueHandle, &music_msg, portMAX_DELAY);
					ui_nowplaying_set(selected_song, // tell UI renderer (not sure if needed here)-----------------------
							song_list[selected_song]);

				}

				else if (msg.evt == EVT_BTN_PREV) {
					//TODO:move PREV song
					ui_nowplaying_skip(-1);
					uint8_t selected_song = ui_get_selected_index();
					music_msg.comm = EVT_NEXT;
					music_msg.data = selected_song + 1;
					xQueueSend(musicQueueHandle, &music_msg, portMAX_DELAY);
					ui_nowplaying_set(selected_song, // tell UI renderer (not sure if needed here)-----------------------
							song_list[selected_song]);

				}

				else if (msg.evt == EVT_BTN_PLAY) {
					if (play_state) {
						play_state = 0; // song paused
						music_msg.data = 0;
						music_msg.comm = EVT_PAUSE;
						xQueueSend(musicQueueHandle, &music_msg, portMAX_DELAY);
						ui_nowplaying_toggle_pause();
					} else {
						play_state = 1;
						music_msg.data = 0;
						music_msg.comm = EVT_RESUME;
						xQueueSend(musicQueueHandle, &music_msg, portMAX_DELAY);
						ui_nowplaying_toggle_pause();
					}

				}

				else if (msg.evt == EVT_BTN_TIMER) {

					//set playing song as default timer lullaby
					currentOverlay.type = OVERLAY_TIMER;
					currentState = UI_TIMER_NOWPLAYING;

				}

				else if (msg.evt == EVT_BTN_VOL_UP
						|| msg.evt == EVT_BTN_VOL_DOWN) {
					volume(msg.evt);

				}

				break;

			case UI_STATE_NOWPLAYING_BLE:
				if (msg.evt == EVT_BTN_MENU) {
					currentState = UI_STATE_MENU;
					music_msg.comm = EVT_BLE_OFF;
					music_msg.data = 0; //if data == 0 then turn off ble
					xQueueSend(musicQueueHandle, &music_msg, portMAX_DELAY);

				}
				break;

			case UI_TIMER:
				switch (msg.evt) {
				case EVT_BTN_TIMER:
					ui_timer_navigate(+1);

					break;
				case EVT_BTN_PLAY:

					uint8_t timer_value = ui_get_timer_minutes();

					timer_counter(timer_value);

					music_msg.comm = EVT_PLAY;
					music_msg.data = saved_song + 1;
					xQueueSend(musicQueueHandle, &music_msg, portMAX_DELAY);
					ui_nowplaying_set(saved_song,        // tell UI renderer
							song_list[saved_song]);
					currentState = UI_STATE_NOWPLAYING_DF;
					currentOverlay.type = OVERLAY_NONE;
					break;
				}
				break;

			case UI_TIMER_NOWPLAYING:
				if (msg.evt == EVT_BTN_TIMER) {

					ui_timer_navigate(+1);
				}

				else if (msg.evt == EVT_BTN_PLAY) {
					//set playing song as default timer lullaby
					saved_song = ui_get_selected_index();
					uint8_t timer_value = ui_get_timer_minutes();
					timer_counter(timer_value);

					music_msg.comm = EVT_PLAY;
					music_msg.data = saved_song + 1;
					xQueueSend(musicQueueHandle, &music_msg, portMAX_DELAY);
					ui_nowplaying_set(saved_song,        // tell UI renderer
							song_list[saved_song]);
					currentOverlay.type = OVERLAY_NONE;
					currentState = UI_STATE_NOWPLAYING_DF;
				}

				else if (msg.evt == EVT_BTN_VOL_UP || EVT_BTN_VOL_DOWN) {

					volume(msg.evt);
				}
				break;

			case UI_LIGHT_LIST:
				if ((osKernelGetTickCount() - lightOverlay_open_tick)
						>= pdMS_TO_TICKS(LIGHT_OVERLAY_PERIOD_MS)) {
					currentState = previousState;
					currentOverlay.type = OVERLAY_NONE;

				} else if (msg.evt == EVT_BTN_LIGHT) {
					ui_light_navigate(1);
					lightOverlay_open_tick = osKernelGetTickCount();
					xQueueSend(lightQueueHandle, (int* )ui_get_light_mode(),
							portMAX_DELAY);
				}
				break;

			default:
				break;

			}

			if (msg.evt == EVT_BTN_LIGHT) {
				currentOverlay.type = OVERLAY_LIGHT_MENU;
				previousState = currentState;
				currentState = UI_LIGHT_LIST;
				lightOverlay_open_tick = osKernelGetTickCount();

			}

			if (vol_flag == 1) {
				if ((osKernelGetTickCount() - volOverlay_open_tick)
						>= pdMS_TO_TICKS(VOL_OVERLAY_PERIOD_MS)) {
					currentOverlay.type = previousOverlay.type;
					vol_flag = 0;
				}

			}

			if (timer_flag == 1) {
				if ((osKernelGetTickCount() - timer_start)
						>= pdMS_TO_TICKS(timeout_ms)) {
					timer_flag = 0;

					music_msg.comm = EVT_STOP;
					music_msg.data = 0;
					xQueueSend(musicQueueHandle, &music_msg, portMAX_DELAY);
				}
			}

			if (currentOverlay.type = OVERLAY_ALARM_DELETE) {
				if (msg.evt == EVT_BTN_NEXT) {
					ui_alarm_delete_navigate(1);
				} else if (msg.evt == EVT_BTN_PREV) {
					ui_alarm_delete_navigate(-1);
				} else if (msg.evt == EVT_BTN_PLAY) {
					ui_alarm_delete_confirm();
					currentOverlay.type == OVERLAY_NONE;
				}
			}

			live_data_fill();
			ui_renderer_update(currentState, &currentOverlay);

		}
	}
}

