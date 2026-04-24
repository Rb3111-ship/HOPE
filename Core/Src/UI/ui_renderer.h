/*
 * ui_renderer.h
 *
 *  Created on: 23 Apr 2026
 *      Author: whp27
 */

#ifndef SRC_UI_UI_RENDERER_H_
#define SRC_UI_UI_RENDERER_H_
#include "ui_state.h"
void ui_renderer_update(ui_state_t currentState, overlay_t currentOverlay);
void UI_DrawMenu();
void UI_DrawPlayDisplay();
void UI_DrawTimerOverlay();
void UI_DrawLightsOverlay();
void UI_DrawTimeSetup();
void UI_DrawMainScreen();


#endif /* SRC_UI_UI_RENDERER_H_ */
