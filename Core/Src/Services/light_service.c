/*
 * light_service.c
 *
 * Used to control the light ring
 *
 * Created on: 21 Apr 2026
 * Author: whp27
 */

#include "light_service.h"
#include "WS2812_driver.h"
#include <stdint.h>
#include "light.h"
#include <math.h>
#include <stdlib.h>
//#include "time_service.h"

#define NUM_LEDS WS2812_NUM_LEDS

//led_t leds[NUM_LEDS];
static led_t *leds = NULL; // Destination buffer link pointing directly to hardware driver arrays
static uint8_t reset = 0;  // Synchronised mode parameter reset tracking latch

// TODO:ADD THE NEW LIGHT MODES AND OFF INTO THE UI_RENDERER; EXCEPT FOR ALARM

/**
 * @brief Pre-loads framework bindings mapping service components onto hardware contexts.
 */
void light_service_init(void) {
	ws2812_start();                  // Direct initialisation call starting physical timer channels
	leds = ws2812_get_frame_buffer(); // Bind target reference tracking pointer onto driver data structures
}

/**
 * @brief Signals a mode switch execution event to synchronise local step parameters.
 */
void light_mode_reset(void) {
	reset = 1;
}

/**
 * @brief Renders a low intensity static white/amber baseline combination.
 */
void light_mode_moonlight(void) {
	/*
	 * Very dim cool/warm mixed light.
	 * Intended to be calm and barely visible.
	 */

	for (int i = 0; i < NUM_LEDS; i++) {
		leds[i].r = 5;
		leds[i].g = 2;
		leds[i].b = 1;
	}
}

/**
 * @brief Emulates a random glimmer effect using statistical scaling sweeps.
 */
void light_mode_starry(void) {
	for (int i = 0; i < NUM_LEDS; i++) {
		if (rand() % 100 < 5) {
			leds[i].r = 20;
			leds[i].g = 20;
			leds[i].b = 40;
		} else {
			leds[i].r = 0;
			leds[i].g = 0;
			leds[i].b = 2;
		}
	}
}

/**
 * @brief Transitions full array parameters uniformly using mathematical HSV mappings.
 */
void light_mode_cycle(void) {

	/*
	 * Smoothly cycles through colors using HSV hue rotation.
	 *
	 * Much smoother and prettier than manually changing RGB.
	 */

	static uint16_t hue = 0;

	hue = (hue + 1) % 360;

	float h = hue;
	float s = 1.0f;
	float v = 0.15f; // Clamped soft peak limits keeping intensity baby-friendly

	float c = v * s;
	float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2) - 1.0f));
	float m = v - c;

	float rf = 0;
	float gf = 0;
	float bf = 0;

	if (h < 60) {
		rf = c;
		gf = x;
		bf = 0;
	} else if (h < 120) {
		rf = x;
		gf = c;
		bf = 0;
	} else if (h < 180) {
		rf = 0;
		gf = c;
		bf = x;
	} else if (h < 240) {
		rf = 0;
		gf = x;
		bf = c;
	} else if (h < 300) {
		rf = x;
		gf = 0;
		bf = c;
	} else {
		rf = c;
		gf = 0;
		bf = x;
	}

	uint8_t r = (uint8_t) ((rf + m) * 255);
	uint8_t g = (uint8_t) ((gf + m) * 255);
	uint8_t b = (uint8_t) ((bf + m) * 255);

	/*
	 * Apply same colour to entire ring
	 */
	for (int i = 0; i < NUM_LEDS; i++) {
		leds[i].r = r;
		leds[i].g = g;
		leds[i].b = b;
	}
}

/**
 * @brief Deploys maximum hardware intensity assignments tracking constant white parameters.
 */
void light_mode_torch(void) {

	for (int i = 0; i < NUM_LEDS; i++) {
		leds[i].r = 255;
		leds[i].g = 180;
		leds[i].b = 80;
	}
//	set_led_data(leds);

}

/**
 * @brief Renders a non-aggressive soft pulsing red warning tracking custom state time lines.
 */
void light_mode_alarm(void) {
	/*
	 * Gentle pulsing alarm.
	 *
	 * NOT aggressive flashing.
	 * Slowly pulses warm orange/red light.
	 */

	static int brightness = 10;
	static int direction = 1;

	// Reset execution variables upon entering the state
	if (reset == 1) {
		brightness = 10;
		direction = 1;
		reset = 0;
	}

	brightness += direction * 2;

	// Clamp boundaries and invert direction properties appropriately
	if (brightness >= 180)
		direction = -1;

	if (brightness <= 10)
		direction = 1;

	for (int i = 0; i < NUM_LEDS; i++) {
		leds[i].r = brightness;
		leds[i].g = brightness / 4;
		leds[i].b = 0;
	}
}

/**
 * @brief Multi-phase progressive sunrise cycle transitions from deep red to bright white.
 */
void light_mode_sunrise(void) {
	/*
	 * Simulates sunrise:
	 *
	 * Phase 1:
	 * Deep red
	 *
	 * Phase 2:
	 * Orange
	 *
	 * Phase 3:
	 * Warm white
	 *
	 * Runs continuously.
	 */

	static uint32_t tick = 0;

	tick++;

	// Enforce baseline initialisation states if reset triggers are flag active
	if (reset == 1) {
		tick = 0;
		reset = 0;
	}
	/*
	 * Controls speed.
	 * Increase divisor for slower sunrise.
	 */
	uint32_t phase = (tick / 20) % 768;

	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;

	if (phase < 256) {
		/*
		 * Red rising
		 */
		r = phase;
		g = 0;
		b = 0;
	} else if (phase < 512) {
		/*
		 * Orange transition
		 */
		r = 255;
		g = phase - 256;
		b = 0;
	} else {
		/*
		 * Warm white transition
		 */
		r = 255;
		g = 255;
		b = (phase - 512) / 4;
	}

	for (int i = 0; i < NUM_LEDS; i++) {
		leds[i].r = r;
		leds[i].g = g;
		leds[i].b = b;
	}
}

/**
 * @brief Slowly steps structural illumination values down to zero over elapsed time.
 */
void light_mode_night_fade(void) {

	/*
	 * Starts moderately bright
	 * then slowly fades toward darkness.
	 *
	 * Useful as a sleep timer.
	 */
	/*
	 * Fade extremely slowly.
	 * Called every ~20ms from task.
	 */
	static uint32_t slowCounter = 0;

	slowCounter++;
	static int brightness = 180;

	// Force execution parameters to clean entry levels upon system initialisation calls
	if (reset == 1) {
		brightness = 180; // Corrected initial state assignments matching context requirements
		reset = 0;
		slowCounter = 0;
	}

	// Throttle down scalar tracking steps to achieve slow visual reductions
	if (slowCounter >= 50) {
		slowCounter = 0;

		if (brightness > 2) {
			brightness--;
		}
	}

	for (int i = 0; i < NUM_LEDS; i++) {
		/*
		 * Warm dim amber
		 */
		leds[i].r = brightness;
		leds[i].g = brightness / 3;
		leds[i].b = 0;
	}
}

/**
 * @brief Regular sin-style brightness modification routines tracking single baseline colors.
 */
void light_mode_breathing(void) {

	static int brightness = 0;
	static int dir = 1;

	// Re-zero Localised properties upon structural switch entries
	if (reset == 1) {
		brightness = 0;
		dir = 1;
		reset = 0;
	}

	brightness += dir;

	// Check threshold bounds limits and toggle transformation step metrics
	if (brightness >= 100)
		dir = -1;

	if (brightness <= 5)
		dir = 1;

	for (int i = 0; i < NUM_LEDS; i++) {
		leds[i].r = brightness;
		leds[i].g = brightness / 2;
		leds[i].b = 0;
	}
}

/**
 * @brief Forces zero value assignments across all array indices to turn the LED array completely off.
 */
void light_clear(void) {
	for (int i = 0; i < NUM_LEDS; i++) {
		leds[i].r = 0;
		leds[i].g = 0;
		leds[i].b = 0;
	}
}

/**
 * @brief Structural switch router executing explicit subroutines matching incoming tracking states.
 * @param light_state Evaluated target mode state.
 */
void lightRenderer_update(light_state_t light_state) {
	switch (light_state) {
	case MOONLIGHT_STATE:
		light_mode_moonlight();
		break;

	case STARRY_STATE:
		light_mode_starry();
		break;

	case BREATHING_STATE:
		light_mode_breathing();
		break;

	case CYCLE_STATE:
		light_mode_cycle();
		break;

	case TORCH_STATE:
		light_mode_torch();
		break;

	case ALARM_STATE:
		light_mode_alarm();
		break;

	case SUNRISE_STATE:
		light_mode_sunrise();
		break;

	case NIGHTFADE_STATE:
		light_mode_night_fade();
		break;

	case OFF_STATE:
		light_clear();
		break;
	}

}
