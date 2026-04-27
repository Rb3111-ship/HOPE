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
static overlay_t currentOverlay = { .active = false, .type = OVERLAY_NONE,
		.timeout_ms = 0 };
static music_msg_t music_msg;

#define DEBOUNCE_DELAY_MS 50

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
					// TODO:update renderer
				} else if (msg.evt == EVT_BTN_TIMER) {
					currentOverlay.active = true;
					currentOverlay.type = OVERLAY_TIMER;
					currentOverlay.timeout_ms = 10000;
					// TODO:update renderer
				} else if (msg.evt == EVT_SENSOR_DATA_READY) {
					// TODO:get new sensor data and update display

				}
				break;

			case UI_STATE_MUSIC_MENU:
				if (msg.evt == EVT_BTN_MENU) {
					currentState = UI_STATE_MAIN;
				} else if (msg.evt == EVT_BTN_NEXT) {
					//TODO: move down menu
					ui_menu_navigate(1);
				}

				else if (msg.evt == EVT_BTN_PREV) {
					// TODO:move up menu
					ui_menu_navigate(-1);
				}

				else if (msg.evt == EVT_BTN_PLAY) {
					// TODO:open current menu item
					switch (ui_get_menu_icon()) {
					case 0:
						currentState = UI_STATE_MUSIC_LIST;
						break;
					case 1:
						currentState = UI_STATE_NOWPLAYING_BLE;
						break;
					case 2:
						currentState = UI_STATE_TIME_SETUP;
						break;
					}
				}

				break;
			case UI_STATE_MUSIC_LIST:
				if (msg.evt == EVT_BTN_MENU) {
					currentState = UI_STATE_MUSIC_MENU;
					music_msg.comm = EVT_STOP;
					xQueueSend(musicQueueHandle, &music_msg, portMAX_DELAY);

				} else if (msg.evt == EVT_BTN_NEXT) {
					// move down list
					ui_song_list_navigate(1);
				}

				else if (msg.evt == EVT_BTN_PREV) {
					//move up list
					ui_song_list_navigate(-1);
				} else if (msg.evt == EVT_BTN_PLAY) {

					uint8_t selected_song = ui_get_selected_index();
					music_msg.comm = EVT_PLAY;
					music_msg.data = selected_song + 1;
					xQueueSend(musicQueueHandle, &music_msg, portMAX_DELAY);
					ui_nowplaying_set(selected_song,         // tell UI renderer
							song_list[selected_song]);
					currentState = UI_STATE_NOWPLAYING_DF;
				}

				break;
			case UI_STATE_NOWPLAYING_DF:
				if (msg.evt == EVT_BTN_MENU) {
					currentState = UI_STATE_MUSIC_LIST;
				} else if (msg.evt == EVT_BTN_NEXT) {
					// TODO:move NEXT song
				}

				else if (msg.evt == EVT_BTN_PREV) {
					//TODO:move PREV song
				} else if (msg.evt == EVT_BTN_PLAY) {
					//TODO:if song playing PAUSE the song else if paused then PLAY
					music_msg.comm = EVT_TOGGLE_PAUSE;
					xQueueSend(musicQueueHandle, &music_msg, portMAX_DELAY);

				}

				else if (msg.evt == EVT_BTN_TIMER) {

					//TODO:set playing song as default timer lullaby
					currentOverlay.active = true;
					currentOverlay.type = OVERLAY_TIMER;
					currentOverlay.timeout_ms = 10000;
				}

				break;
			case UI_STATE_NOWPLAYING_BLE:
				if (msg.evt == EVT_BTN_MENU) {
					currentState = UI_STATE_MUSIC_MENU;
					//TODO: turn off ble music
				}
				break;

			case UI_STATE_TIME_SETUP:
				if (msg.evt == EVT_BTN_MENU) {
					//TODO: get the hour and min from rtc before going to time setup
					uint8_t h, m; //place holders
					ui_time_setup_seed(h, m);
					currentState = UI_STATE_MUSIC_MENU;

				}

				if (msg.evt == EVT_BTN_PREV) {

				}

				if (msg.evt == EVT_BTN_NEXT) {

				}

				// TODO: add actions for time changing
				break;

			default:
				break;

			}

			if (msg.evt == EVT_BTN_LIGHT) {
				currentOverlay.active = true;
				currentOverlay.type = OVERLAY_LIGHT_MENU;
				currentOverlay.timeout_ms = 10000;
			}

			if (msg.evt == EVT_BTN_VOL_UP || msg.evt == EVT_BTN_VOL_DOWN) {
				currentOverlay.active = true;

				// TODO: Send command to Music Task to adjust physical volume
			}

		}

		live_data_fill();
		ui_renderer_update(currentState, &currentOverlay);

	}
}

