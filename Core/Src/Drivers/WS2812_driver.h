/*
 * WS2812_driver.h
 *
 * Created on: 13 May 2026
 * Author: whp27
 */

#ifndef SRC_DRIVERS_WS2812_DRIVER_H_
#define SRC_DRIVERS_WS2812_DRIVER_H_

#include <stdint.h>

#define WS2812_NUM_LEDS 16   // LEDs in the ring

/**
 * @brief Compound configuration structure outlining individual discrete primary color parameters.
 */
typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} led_t;

//void set_led_data(led_t *set_led);
void ws2812_start(void);
led_t *ws2812_get_frame_buffer(void);

#endif /* SRC_DRIVERS_WS2812_DRIVER_H_ */
