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

	for (;;) {

		if (xQueueReceive(uiQueueHandle, &msg, portMAX_DELAY) == pdPASS) {

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
				}

				else if (msg.evt == EVT_BTN_PREV) {
					// TODO:move up menu
				}

				else if (msg.evt == EVT_BTN_PLAY) {
					// TODO:open current menu item
				}

				break;
			case UI_STATE_MUSIC_LIST:
				if (msg.evt == EVT_BTN_MENU) {
					currentState = UI_STATE_MUSIC_MENU;
					// TODO:if music was playing the STOP
				} else if (msg.evt == EVT_BTN_NEXT) {
					// TODO:move down list
				}

				else if (msg.evt == EVT_BTN_PREV) {
					//TODO:move up list
				} else if (msg.evt == EVT_BTN_PLAY) {
					//TODO:PLAY current song on list
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
					currentState = UI_STATE_MUSIC_MENU;

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
				currentOverlay.type = OVERLAY_VOLUME;
				// TODO: Send command to Music Task to adjust physical volume
			}

			ui_renderer_update(currentState, currentOverlay);
		}

	}
}

