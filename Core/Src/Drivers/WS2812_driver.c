/*
 * WS2812_driver.c
 *
 * Created on: 13 May 2026
 * Author: whp27
 */
#include "WS2812_driver.h"
#include <stdint.h>
#include "main.h"

/* Framework Layout and Dimensional Constraints */
#define NUM_LEDS 16
#define DMA_BUFFER_SIZE 464  // 24 bits * 16 LEDs = 384 + 80 trailing reset padding slots
#define WS2812_PERIOD 125
#define TOTAL_BITS (NUM_LEDS * 24)
#define HALF_SIZE (DMA_BUFFER_SIZE / 2)

/* Peripheral Capture Compare Values mapping logical bit timing constraints */
#define WS2812_HIGH   80     // Logic high representation (~0.8 us active window)
#define WS2812_LOW    40     // Logic low representation (~0.4 us active window)

/* Core Driver Variables and Hardware Reference Handles */
static uint16_t dmaBuffer[DMA_BUFFER_SIZE]; // Double-buffered raw circular DMA targets
static led_t leds[NUM_LEDS];                // Driver context holding immediate pixel properties
volatile uint32_t streamIndex = 0;          // Bit indexing cursor processed inside ISR
extern TIM_HandleTypeDef htim1;

/**
 * @brief Exposes raw memory reference points to upper layer service engines.
 * @return Pointer targeting the underlying contiguous led_t array.
 */
led_t* ws2812_get_frame_buffer(void) {
	return leds;
}

/**
 * @brief Serialisation translation state calculations for the bit-stream stream.
 * @details Invoked inside half/full complete interrupt contexts to populate the next half buffer.
 * @return Raw CCR matching configuration pulse widths.
 */
static uint16_t ws2812_next_pwm_value(void) {
	// Parse if current index falls inside reset frame window parameters
	if (streamIndex >= TOTAL_BITS) {
		streamIndex++;

		if (streamIndex >= DMA_BUFFER_SIZE) {
			streamIndex = 0; // Wrap back around to baseline index initialisation values
		}

		return 0; // Output a 0% duty-cycle reset latch pulse
	}

	// Calculate target array mapping offsets based on current bit position properties
	uint32_t ledIndex = streamIndex / 24;
	uint32_t bitIndex = streamIndex % 24;

	led_t *led = &leds[ledIndex];

	// Pack discrete colors into standard WS2812 GRB data arrangements
	uint32_t color = (led->g << 16) | (led->r << 8) | led->b;

	// Trace a single target evaluation mask down across the standard 24-bit architecture
	uint32_t mask = 1 << (23 - bitIndex);

	streamIndex++;

	// Direct pulse evaluation modifications based on current mask assignments
	if (color & mask)
		return WS2812_HIGH;
	else
		return WS2812_LOW;
}

/**
 * @brief Populates a target buffer segment with configured serialisation elements.
 * @param buf Target buffer destination structure.
 * @param length Element loop processing boundaries.
 */
void ws2812_fill_buffer(uint16_t *buf, uint32_t length) {
	for (uint32_t i = 0; i < length; i++) {
		buf[i] = ws2812_next_pwm_value();
	}
}

/**
 * @brief Half-Complete Interrupt Callback handler triggered via DMA peripheral channels.
 */
void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef *htim) {
	ws2812_fill_buffer(&dmaBuffer[0], HALF_SIZE); // Safe to modify initial lower sectors
}

/**
 * @brief Transfer-Complete Interrupt Callback handler triggered via DMA peripheral channels.
 */
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
	ws2812_fill_buffer(&dmaBuffer[HALF_SIZE], HALF_SIZE); // Safe to modify remaining upper sectors
}

/**
 * @brief Arm and deploy hardware timers and associated circular background DMA engines.
 */
void ws2812_start(void) {
	streamIndex = 0;

	// Initialise the double buffer regions with raw bootstrap configurations
	ws2812_fill_buffer(&dmaBuffer[0], HALF_SIZE);
	ws2812_fill_buffer(&dmaBuffer[HALF_SIZE], HALF_SIZE);

	// Start circular timer PWM generation matching physical driver configuration constraints
	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t*) dmaBuffer,
	DMA_BUFFER_SIZE);
}

/*
 * WS2812 Timing Calculation
 *
 * WS2812 bit rate:
 * 800 kHz
 *
 * One bit period:
 * T = 1 / 800000
 * = 1.25 us
 *
 * Timer clock:
 * 100 MHz
 *
 * Timer tick:
 * 1 / 100000000
 * = 10 ns
 *
 * Required timer counts per WS2812 bit:
 * 1.25 us / 10 ns
 * = 125 ticks
 *
 * Therefore:
 * ARR = 124
 *
 * WS2812 logic timings:
 *
 * Logic 1:
 * High time ≈ 0.8 us
 * 0.8 us / 10 ns = 80 ticks
 *
 * Logic 0:
 * High time ≈ 0.4 us
 * 0.4 us / 10 ns = 40 ticks
 *
 * Final values:
 * Logic 1 CCR = 80
 * Logic 0 CCR = 40
 *
 * PWM frequency:
 * 100 MHz / 125
 * = 800 kHz
 */
