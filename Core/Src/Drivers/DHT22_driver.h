/*
 * DHT22_driver.h
 *
 * Created on: 29 Apr 2026
 * Author: whp27
 */

#ifndef SRC_DRIVERS_DHT22_DRIVER_H_
#define SRC_DRIVERS_DHT22_DRIVER_H_
#include <stdint.h>

/**
 * @brief Error code enumerations evaluating line parsing consistency blocks.
 */
typedef enum{
	OK,
	TIMEOUT,
	CHECKSUM_ERROR
}dht22_status_t;

/**
 * @brief Tracking flags steering internal sequence processing steps within the line capture state machine.
 */
typedef enum{
	WAIT_RESPONSE_LOW,   // first 80 µs LOW
	WAIT_RESPONSE_HIGH,  // 80 µs HIGH
	WAIT_BIT_RISE,       // start of bit HIGH
	WAIT_BIT_FALL, // end of bit HIGH → measure
	DONE
}pulse_state_t;

// Low-Level Hardware Driver Application Programming Interface Prototypes
void DHT22_init(void);
dht22_status_t dht22_read(uint8_t *out);
#endif /* SRC_DRIVERS_DHT22_DRIVER_H_ */
