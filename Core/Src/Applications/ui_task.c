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

static ui_state_t currentState = UI_STATE_MAIN;
static ui_state_t previousState;
static overlay_type_t previousOverlay ;
static overlay_t currentOverlay = {  .type = OVERLAY_NONE,
		.timeout_ms = 0 };
static music_msg_t music_msg;
uint8_t saved_song = 0; //default saved song is song 1
static uint32_t lightOverlay_open_tick;
static uint32_t volOverlay_open_tick;

#define DEBOUNCE_DELAY_MS 50
#define LIGHT_OVERLAY_PERIOD_MS 5000
#define VOL_OVERLAY_PERIOD_MS 1500

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

void ui_Task(void *pvParameters) {

	ui_msg_t msg;
	const TickType_t xDelay100ms = pdMS_TO_TICKS(100UL);

	for (;;) {

		if (xQueueReceive(uiQueueHandle, &msg, xDelay100ms) == pdPASS) {

			switch (currentState) {

			case UI_STATE_MAIN:
				if (msg.evt == EVT_BTN_MENU) {
					currentState = UI_STATE_MUSIC_MENU;

				}

				else if (msg.evt == EVT_BTN_TIMER) {
					currentOverlay.type = OVERLAY_TIMER;
					currentState = UI_TIMER;

				} else if (msg.evt == EVT_SENSOR_DATA_READY) {
					// TODO:get new sensor data and update display

				}
				break;

			case UI_STATE_MUSIC_MENU:
				if (msg.evt == EVT_BTN_MENU) {
					currentState = UI_STATE_MAIN;
				}

				else if (msg.evt == EVT_BTN_NEXT) {
					//TODO: move down menu
					ui_menu_navigate(1);
				}

				else if (msg.evt == EVT_BTN_PREV) {
					// TODO:move up menu
					ui_menu_navigate(-1);
				}

				else if (msg.evt == EVT_BTN_PLAY) {

					switch (ui_get_menu_icon()) {
					case 0:
						currentState = UI_STATE_MUSIC_LIST;
						break;
					case 1:
						currentState = UI_STATE_NOWPLAYING_BLE;
						music_msg.comm = EVT_BLE_ON;
						music_msg.data = 0;
						music_msg.time = 0;
						xQueueSend(musicQueueHandle, &music_msg, portMAX_DELAY);

						break;
					case 2:
						currentState = UI_STATE_TIME_SETUP;
						ui_time_setup_seed(h, m);
						break;
					}
				}

				break;

			case UI_STATE_MUSIC_LIST:
				if (msg.evt == EVT_BTN_MENU) {
					currentState = UI_STATE_MUSIC_MENU;
					music_msg.comm = EVT_STOP;
					music_msg.data = 0;
					music_msg.time = 0; //stops timer function when you exit music
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
					music_msg.time = 0;
					xQueueSend(musicQueueHandle, &music_msg, portMAX_DELAY);
					ui_nowplaying_set(selected_song,         // tell UI renderer
							song_list[selected_song]);
					currentState = UI_STATE_NOWPLAYING_DF;
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
					uint8_t selected_song = ui_get_selected_index();
					music_msg.data = selected_song + 1;
					music_msg.comm = EVT_TOGGLE_PAUSE;
					music_msg.time = 0;
					xQueueSend(musicQueueHandle, &music_msg, portMAX_DELAY);

				}

				else if (msg.evt == EVT_BTN_TIMER) {

					//set playing song as default timer lullaby
					currentOverlay.type = OVERLAY_TIMER;
					currentState = UI_TIMER_NOWPLAYING;

				}

				break;

			case UI_STATE_NOWPLAYING_BLE:
				if (msg.evt == EVT_BTN_MENU) {
					currentState = UI_STATE_MUSIC_MENU;
					music_msg.comm = EVT_BLE_OFF;
					music_msg.time = 0u;
					music_msg.data = 0; //if data == 0 then turn off ble
					xQueueSend(musicQueueHandle, &music_msg, portMAX_DELAY);

				}
				break;

			case UI_STATE_TIME_SETUP:
				if (msg.evt == EVT_BTN_MENU) {
					currentState = UI_STATE_MUSIC_MENU;

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
					currentState = UI_STATE_MUSIC_MENU;
				}

				break;

			case UI_TIMER:
				switch (msg.evt) {
				case EVT_BTN_TIMER:
					ui_timer_navigate(+1);

					break;
				case EVT_BTN_PLAY:

					uint8_t timer_value = ui_get_timer_minutes();

					music_msg.comm = EVT_PLAY;
					music_msg.data = saved_song + 1;
					music_msg.time = timer_value; // if timer is 0 deal with it in the music task to stop any active timers
					xQueueSend(musicQueueHandle, &music_msg, portMAX_DELAY);
					ui_nowplaying_set(saved_song,        // tell UI renderer
							song_list[saved_song]);
					currentState = UI_STATE_NOWPLAYING_DF;
					currentOverlay.type = OVERLAY_NONE;
					break;
				}
				break;

			case UI_TIMER_NOWPLAYING:
				switch (msg.evt) {
				case EVT_BTN_TIMER:
					ui_timer_navigate(+1);
					break;

				case EVT_BTN_PLAY:
					//set playing song as default timer lullaby
					saved_song = ui_get_selected_index();
					uint8_t timer_value = ui_get_timer_minutes();

					music_msg.comm = EVT_TIMER;
					music_msg.time = timer_value;
					xQueueSend(musicQueueHandle, &music_msg, portMAX_DELAY); //current song keeps playing
					currentOverlay.type = OVERLAY_NONE;
					break;
				}

			case UI_LIGHT_LIST:
				if ((osKernelGetTickCount() - lightOverlay_open_tick)
						>= pbMS_TO_TICKS(LIGHT_OVERLAY_PERIOD_MS)) {
					currentState = previousState;
					currentOverlay.type = OVERLAY_NONE;

				} else if (msg.evt == EVT_BTN_LIGHT) {
					ui_light_navigate(1);
					lightOverlay_open_tick = osKernelGetTickCount();
					xQueueSend(lightQueueHandle, ui_get_light_mode(),
							portMax_DELAY);
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

			if (msg.evt == EVT_BTN_VOL_UP || msg.evt == EVT_BTN_VOL_DOWN) {
				uint8_t set_Vol = 0;
				volOverlay_open_tick = osKernelGetTickCount();
				previousOverlay = currentOverlay;
				if (msg.evt == EVT_BTN_VOL_UP) {
					currentOverlay.type = OVERLAY_VOLUME_UP;
					set_Vol = 1;
				} else if (msg.evt == EVT_BTN_VOL_DOWN) {
					currentOverlay.type = OVERLAY_VOLUME_DOWN;
					set_Vol = -1;
				}
				music_msg.comm = EVT_SET_VOL;
				music_msg.time = 0u;
				music_msg.data = set_Vol;
				xQueueSend(musicQueueHandle, &music_msg, portMAX_DELAY);

			}

			if ((osKernelGetTickCount() - lightOverlay_open_tick)
					>= pbMS_TO_TICKS(VOL_OVERLAY_PERIOD_MS)) {
				currentOverlay.type = previousOverlay;
			}

			live_data_fill();
			ui_renderer_update(currentState, &currentOverlay);

		}
	}
}

