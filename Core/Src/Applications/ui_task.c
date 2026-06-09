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
#include "light.h"

static ui_state_t currentState = UI_STATE_MAIN;
static ui_state_t previousState;
static ui_state_t previousStateAlarm;
static overlay_t previousOverlay;
static overlay_t currentOverlay = { .type = OVERLAY_NONE };
static music_msg_t music_msg;
static light_msg_t light_msg;
static uint8_t saved_song = 0; //default saved song is song 1
static uint32_t lightOverlay_open_tick;
static uint32_t volOverlay_open_tick;
static uint32_t timer_start;
static uint8_t play_state = 0;
static uint8_t set_Vol = 5;
static uint8_t vol_flag = 0;
static uint8_t timer_flag = 0;
static uint32_t timeout_ms = 0;
static uint32_t alarm_time = 0;
static uint8_t lightOverlay = 0;

#define LIGHT_OVERLAY_PERIOD_MS 5000
#define ALARM_OVERLAY_PERIOD  60000
#define VOL_OVERLAY_PERIOD_MS 1500
#define FIVE_MIN_PERIOD_MS 300000
#define TEN_MIN_PERIOD_MS 600000
#define FIFTEEN_MIN_PERIOD_MS 900000
#define THIRTY_MIN_PERIOD_MS 1800000
#define SIXTY_MIN_PERIOD_MS 3600000
#define ALARM_TONE 26
#define ALARM_LIGHT_MODE 8

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) { // called automatically by HAL when EXTI interrupt occurs
	BaseType_t xHigherPriorityTaskWoken = pdFALSE; //GPIO_Pin used to compare with pins used for touch sensors
	ui_msg_t msg = { 0 };

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
	default:
		return;
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
		if (set_Vol < 30)
			set_Vol++;
	} else if (msg == EVT_BTN_VOL_DOWN) {
		currentOverlay.type = OVERLAY_VOLUME_DOWN;
		if (set_Vol > 0)
			set_Vol--;
	}

	setVolume(set_Vol);
	music_msg.comm = EVT_SET_VOL;
	music_msg.data = set_Vol;
	if (xQueueSend(musicQueueHandle, &music_msg, pdMS_TO_TICKS(10)) != pdPASS) {

	}
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

void stop_Alarm() {

	music_msg.comm = EVT_STOP;
	music_msg.data = 0;
	if (xQueueSend(musicQueueHandle, &music_msg,
			pdMS_TO_TICKS(10)) != pdPASS) {

	}
	currentOverlay.type = OVERLAY_NONE;
	currentState = previousStateAlarm;
	light_msg.mode = 0;
	if (xQueueSend(lightQueueHandle, &light_msg,
			pdMS_TO_TICKS(10)) != pdPASS) {

	}
	if (currentState == UI_STATE_NOWPLAYING_BLE) { // if the current state is ble, turn it on again
		music_msg.comm = EVT_BLE_ON;
		music_msg.data = 0;
		if (xQueueSend(musicQueueHandle, &music_msg,
				pdMS_TO_TICKS(10)) != pdPASS) {

		}
	}

}

void ui_Task(void *pvParameters) {

	ui_msg_t msg;
	const TickType_t xDelay100ms = pdMS_TO_TICKS(100UL);

	for (;;) {

		if (currentState == UI_ALARM_FIRING) {
			uint32_t current_time = osKernelGetTickCount();
			if ((current_time - alarm_time)
					>= pdMS_TO_TICKS(ALARM_OVERLAY_PERIOD)) {
				stop_Alarm(); // Send EVT_STOP, restore previousStateAlarm, reset lights
			}
		}

		if (xQueueReceive(uiQueueHandle, &msg, xDelay100ms) == pdPASS) {

			if (currentState == UI_ALARM_FIRING) {
				if ((msg.evt == EVT_BTN_TIMER || msg.evt == EVT_BTN_PLAY
						|| msg.evt == EVT_BTN_NEXT || msg.evt == EVT_BTN_PREV
						|| msg.evt == EVT_BTN_VOL_UP
						|| msg.evt == EVT_BTN_VOL_DOWN
						|| msg.evt == EVT_BTN_LIGHT || msg.evt == EVT_BTN_MENU)) {
					stop_Alarm();

				}

			} else {

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
							if (xQueueSend(musicQueueHandle, &music_msg,
									pdMS_TO_TICKS(10)) != pdPASS) {

							}
							break;
						case 1:
							currentState = UI_STATE_NOWPLAYING_BLE;
							music_msg.comm = EVT_BLE_ON;
							music_msg.data = 0;
							if (xQueueSend(musicQueueHandle, &music_msg,
									pdMS_TO_TICKS(10)) != pdPASS) {

							}

							break;
						case 2:
							currentState = UI_STATE_TIME_SUBMENU;
						}
					}

					break;

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

					else if (msg.evt == EVT_BTN_PLAY) {
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

					else if (msg.evt == EVT_BTN_PREV) {
						ui_time_setup_adjust(-1);
					}

					else if (msg.evt == EVT_BTN_NEXT) {
						ui_time_setup_adjust(1);
					}

					else if (msg.evt == EVT_BTN_PLAY) {
						ui_time_setup_next_field();
					}

					else if (msg.evt == EVT_BTN_TIMER) {
						ui_time_setup_get();
						currentState = UI_STATE_TIME_SUBMENU;
					}

					break;

				case UI_STATE_ALARM_SETUP:
					if (msg.evt == EVT_BTN_MENU) {
						currentState = UI_STATE_ALARMS_LIST;

					}

					else if (msg.evt == EVT_BTN_PREV) {
						ui_alarm_setup_adjust(-1);
					}

					else if (msg.evt == EVT_BTN_NEXT) {
						ui_alarm_setup_adjust(1);
					}

					else if (msg.evt == EVT_BTN_PLAY) {
						ui_alarm_setup_next_field();
					}

					else if (msg.evt == EVT_BTN_TIMER) {
						ui_alarm_setup_confirm();
						currentState = UI_STATE_ALARMS_LIST;
					}

					break;

				case UI_STATE_MUSIC_LIST:
					if (msg.evt == EVT_BTN_MENU) {
						currentState = UI_STATE_MENU;
						music_msg.comm = EVT_STOP;
						music_msg.data = 0;
						if (xQueueSend(musicQueueHandle, &music_msg,
								pdMS_TO_TICKS(10)) != pdPASS) {

						}

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
						if (xQueueSend(musicQueueHandle, &music_msg,
								pdMS_TO_TICKS(10)) != pdPASS) {

						}
						ui_nowplaying_set(selected_song,     // tell UI renderer
								song_list[selected_song]);
						play_state = 1;     // the song is playing  not paused
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
						if (xQueueSend(musicQueueHandle, &music_msg,
								pdMS_TO_TICKS(10)) != pdPASS) {

						}
						ui_nowplaying_set(selected_song,
								song_list[selected_song]);

					}

					else if (msg.evt == EVT_BTN_PREV) {
						//TODO:move PREV song
						ui_nowplaying_skip(-1);
						uint8_t selected_song = ui_get_selected_index();
						music_msg.comm = EVT_PREV;
						music_msg.data = selected_song + 1;
						if (xQueueSend(musicQueueHandle, &music_msg,
								pdMS_TO_TICKS(10)) != pdPASS) {

						}
						ui_nowplaying_set(selected_song,
								song_list[selected_song]);

					}

					else if (msg.evt == EVT_BTN_PLAY) {
						if (play_state) {
							play_state = 0; // song paused
							music_msg.data = 0;
							music_msg.comm = EVT_PAUSE;
							if (xQueueSend(musicQueueHandle, &music_msg,
									pdMS_TO_TICKS(10)) != pdPASS) {

							}
							ui_nowplaying_toggle_pause();
						} else {
							play_state = 1;
							music_msg.data = 0;
							music_msg.comm = EVT_RESUME;
							if (xQueueSend(musicQueueHandle, &music_msg,
									pdMS_TO_TICKS(10)) != pdPASS) {

							}
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
						music_msg.data = 0;
						if (xQueueSend(musicQueueHandle, &music_msg,
								pdMS_TO_TICKS(10)) != pdPASS) {

						}
					}
					break;

				case UI_TIMER:

					if (msg.evt == EVT_BTN_TIMER) {

						ui_timer_navigate(+1);

					} else if (msg.evt == EVT_BTN_PLAY) {

						uint8_t timer_value = ui_get_timer_minutes();

						timer_counter(timer_value);
						music_msg.comm = EVT_PLAY;
						music_msg.data = saved_song + 1;
						if (xQueueSend(musicQueueHandle, &music_msg,
								pdMS_TO_TICKS(10)) != pdPASS) {

						}
						ui_nowplaying_set(saved_song,        // tell UI renderer
								song_list[saved_song]);
						currentState = UI_STATE_NOWPLAYING_DF;
						currentOverlay.type = OVERLAY_NONE;

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
						if (xQueueSend(musicQueueHandle, &music_msg,
								pdMS_TO_TICKS(10)) != pdPASS) {

						}
						ui_nowplaying_set(saved_song,        // tell UI renderer
								song_list[saved_song]);
						currentOverlay.type = OVERLAY_NONE;
						currentState = UI_STATE_NOWPLAYING_DF;
					}

					else if (msg.evt == EVT_BTN_VOL_UP
							|| msg.evt == EVT_BTN_VOL_DOWN) {

						volume(msg.evt);
					}
					break;

				case UI_LIGHT_LIST:
					if (msg.evt == EVT_BTN_LIGHT) {
						ui_light_navigate(1);
						lightOverlay_open_tick = osKernelGetTickCount();
						light_msg.mode = ui_get_light_mode();
						if (xQueueSend(lightQueueHandle, &light_msg,
								pdMS_TO_TICKS(10)) != pdPASS) {

						}
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
					lightOverlay = 1;
				}

				if (currentOverlay.type == OVERLAY_ALARM_DELETE) {
					if (msg.evt == EVT_BTN_NEXT) {
						ui_alarm_delete_navigate(1);
					} else if (msg.evt == EVT_BTN_PREV) {
						ui_alarm_delete_navigate(-1);
					} else if (msg.evt == EVT_BTN_PLAY) {
						currentOverlay.type = OVERLAY_NONE;
						ui_alarm_delete_confirm();

					}
				}
			}
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
				if (xQueueSend(musicQueueHandle, &music_msg,
						pdMS_TO_TICKS(10)) != pdPASS) {

				}
			}
		}

		if (lightOverlay == 1) {
			if ((osKernelGetTickCount() - lightOverlay_open_tick)
					>= pdMS_TO_TICKS(LIGHT_OVERLAY_PERIOD_MS)) {
				currentState = previousState;
				currentOverlay.type = OVERLAY_NONE;
				lightOverlay = 0;
			}
		}

		if (check_alarm() == true) {
			previousStateAlarm = currentState;
			alarm_time = osKernelGetTickCount();
			if (currentState == UI_STATE_NOWPLAYING_BLE) {
				music_msg.comm = EVT_BLE_OFF;
				music_msg.data = 0;
				if (xQueueSend(musicQueueHandle, &music_msg,
						pdMS_TO_TICKS(10)) != pdPASS) {

				}
			}

			currentOverlay.type = OVERLAY_ALARM_FIRING;
			currentState = UI_ALARM_FIRING;
			music_msg.comm = EVT_PLAY;
			music_msg.data = ALARM_TONE;
			if (xQueueSend(musicQueueHandle, &music_msg,
					pdMS_TO_TICKS(10)) != pdPASS) {

			}
			light_msg.mode = ALARM_LIGHT_MODE;
			if (xQueueSend(lightQueueHandle, &light_msg,
					pdMS_TO_TICKS(10)) != pdPASS) {

			}
		}

		live_data_fill();
		ui_renderer_update(currentState, &currentOverlay);

	}
}
