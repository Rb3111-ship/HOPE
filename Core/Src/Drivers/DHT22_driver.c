/*
 * DHT22_driver.c
 *
 * Created on: 29 Apr 2026
 * Author: whp27
 */
#include "stm32f4xx.h"
#include <string.h>
#include "DHT22_driver.h"
#include <stdint.h>
#include "tasks.h"

/* Shared status and storage parameters accessed inside interrupt subroutines */
volatile uint8_t data_buff[5];
volatile pulse_state_t current_state;
volatile dht22_status_t err_status;
uint16_t capture_now = 0;
uint16_t delta = 0;
uint16_t capture_prev = 0;
uint8_t bit = 0;
uint8_t byte_index = 0;
uint8_t bit_index = 0;

/**
 * @brief Deploys bare-metal register assignments configuring hardware peripherals.
 * @details Configures PB8 to standard push-pull arrangements and prepares Timer 4 input capture configurations.
 */
void DHT22_init() {
	// Enable peripheral system clock distribution channels
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
	RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;

	// PB8 as output (MODER = 01)
	GPIOB->MODER &= ~(3 << (8 * 2));
	GPIOB->MODER |= (1 << (8 * 2));

	// Push-pull configuration
	GPIOB->OTYPER &= ~(1 << 8);

	// High speed setting
	GPIOB->OSPEEDR |= (3 << (8 * 2));

	// No pull-up/down (external resistor assumed)
	GPIOB->PUPDR &= ~(3 << (8 * 2));

	TIM4->PSC = 99;     // 100 MHz / 100 = 1 MHz → 1 µs per tick scaling factor
	TIM4->ARR = 0xFFFF;     // free running configuration

	// Channel 3 as input (CC3S = 01)
	TIM4->CCMR2 &= ~(3 << 0);
	TIM4->CCMR2 |= (1 << 0);

	// Capture on BOTH edges
	TIM4->CCER |= (1 << 8);   // CC3E enable
	TIM4->CCER |= (1 << 9);   // CC3P
	TIM4->CCER |= (1 << 11);  // CC3NP

	// Enable timer peripheral
	TIM4->CR1 |= TIM_CR1_CEN;

	// NVIC routing enablement
	NVIC_EnableIRQ(TIM4_IRQn);
}

/**
 * @brief Dynamic register shifting flipping PB8 pin functions onto alternative Input Capture routing paths.
 */
void set_pin_input() {
	// PB8 → Alternate Function (10)
	GPIOB->MODER &= ~(3 << (8 * 2));
	GPIOB->MODER |= (2 << (8 * 2));

	// AF2 (TIM4 alternative mapping)
	GPIOB->AFR[1] &= ~(0xF << ((8 - 8) * 4));
	GPIOB->AFR[1] |= (2 << ((8 - 8) * 4));
}

/**
 * @brief Blocking, bare-metal high-precision timing utility measuring microsecond durations.
 */
void delay_us(uint32_t us) {
	uint16_t start = TIM4->CNT;
	while ((uint16_t) (TIM4->CNT - start) < us)
		;
}

/**
 * @brief Drives initial wake start signals down physical signal lines to trigger sensor transmissions.
 * @details Pulls line down for 2ms, handles brief high state floats, transitions pin mappings, and clear tracking contexts.
 */
void set_pin_output_low() {

	// PB8 already output
	GPIOB->BSRR = (1 << (8 + 16)); // LOW driving pulse
	delay_us(2000);                // ~2 ms hold duration

	GPIOB->BSRR = (1 << 8);        // HIGH float release
	delay_us(30);                  // 20–40 µs stable relaxation period

	set_pin_input();
	// reset internal timer indices + machine execution flags
	current_state = WAIT_RESPONSE_LOW;
	for (uint8_t i = 0; i < 5; i++) {
		data_buff[i] = 0;
	}
	byte_index = 0;
	bit_index = 0;
	capture_prev = 0;

	// Enable capture interrupt channels
	TIM4->DIER |= TIM_DIER_CC3IE;

	TIM4->CNT = 0;
}

/**
 * @brief Core Input Capture Peripheral Interrupt Handler evaluating signal edge variations.
 * @details Demodulates asynchronous bit streams using relative high-pulse duration thresholds.
 */
void TIM4_IRQHandler(void) {

	if (TIM4->SR & TIM_SR_CC3IF) {

		TIM4->SR &= ~TIM_SR_CC3IF; // Clear event bit matching the interrupt flag

		uint8_t level = (GPIOB->IDR >> 8) & 1;

		if (level == 1) {
			// Rising edge processing subroutines
			if (current_state == WAIT_RESPONSE_HIGH) {

				current_state = WAIT_BIT_RISE;
			}

			else if (current_state == WAIT_BIT_RISE) {
				capture_prev = TIM4->CCR3; // Cache start points tracking logic high widths
				current_state = WAIT_BIT_FALL;
			}

		} else {
			// Falling edge processing subroutines
			if (current_state == WAIT_RESPONSE_LOW) //For initial falling edge verification checks
				current_state = WAIT_RESPONSE_HIGH;

			if (current_state == WAIT_BIT_FALL) {
				capture_now = TIM4->CCR3;
				delta = capture_now - capture_prev; // Unsigned math inherently manages counter overflow wrap errors
				bit = (delta > 50) ? 1 : 0; // High durations > 50 microseconds represent logical 1s

				if (byte_index < 5) {
					// Pack individual bits linearly into data byte offsets
					data_buff[byte_index] |= bit << (7 - bit_index);
					bit_index++;
					current_state = WAIT_BIT_RISE;
					if (bit_index >= 8) {
						bit_index = 0;
						byte_index++;
					}

					// Verify data packet consistency upon collecting all 5 fields
					if (byte_index == 5) {

						uint8_t sum = data_buff[0] + data_buff[1] + data_buff[2]
								+ data_buff[3];
						if ((sum & 0xFF) != data_buff[4]) { // Enforce 8-bit wrap constraints via arithmetic mask
							err_status = CHECKSUM_ERROR;
							for (uint8_t i = 0; i < 5; i++) {
								data_buff[i] = 0;
							}
						} else {
							err_status = OK;
						}

						current_state = DONE;
						TIM4->DIER &= ~TIM_DIER_CC3IE; // Disable capture interrupt until next parse request

					}
				}

			}

		}

	}
}

/**
 * @brief High-level tracking API capturing stream packages from the physical sensor line.
 * @details Coordinates signal triggers and implements active busy-wait loop checks to process timeouts.
 * @param out Destination array reference location targeted to receive raw results.
 * @return Verification status enumeration indicating parsed data integrity properties.
 */
dht22_status_t dht22_read(uint8_t *out) {

	set_pin_output_low();  // start hardware handshake sequence
	uint32_t start = xTaskGetTickCount();

	// Busy-wait polling loop tracking current interrupt progress states
	while (current_state != DONE) {
		if ((xTaskGetTickCount() - start) > pdMS_TO_TICKS(10)) {
			err_status = TIMEOUT;
			TIM4->DIER &= ~TIM_DIER_CC3IE; // Terminate interrupt processing upon timeout
			current_state = DONE;
			return err_status;
		}
	}

	// Flush localised hardware buffer parameters safely back to calling interfaces
	for (uint8_t i = 0; i < 5; i++)
		out[i] = data_buff[i];
	return err_status;
}
