/*
 * DHT22_driver.c
 *
 *  Created on: 29 Apr 2026
 *      Author: whp27
 */
#include "stm32f4xx.h"
#include <string.h>
#include "DHT22_driver.h"
#include <stdint.h>
#include "tasks.h"

uint8_t data_buff[5];
volatile pulse_state_t current_state;
volatile dht22_status_t err_status;
uint16_t capture_now = 0;
uint16_t delta = 0;
uint16_t capture_prev = 0;
uint8_t bit = 0;
uint8_t byte_index = 0;
uint8_t bit_index = 0;

void DHT22_init() {
	// Enable clocks
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
	RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;

	// PB8 as output (MODER = 01)
	GPIOB->MODER &= ~(3 << (8 * 2));
	GPIOB->MODER |= (1 << (8 * 2));

	// Push-pull
	GPIOB->OTYPER &= ~(1 << 8);

	// High speed
	GPIOB->OSPEEDR |= (3 << (8 * 2));

	// No pull-up/down (external resistor assumed)
	GPIOB->PUPDR &= ~(3 << (8 * 2));

	TIM4->PSC = 99;     // 100 MHz / 100 = 1 MHz → 1 µs per tick
	TIM4->ARR = 0xFFFF;     // free running

	// Channel 3 as input (CC3S = 01)
	TIM4->CCMR2 &= ~(3 << 0);
	TIM4->CCMR2 |= (1 << 0);

	// Capture on BOTH edges
	TIM4->CCER |= (1 << 8);   // CC3E enable
	TIM4->CCER |= (1 << 9);   // CC3P
	TIM4->CCER |= (1 << 11);  // CC3NP

	// Enable timer
	TIM4->CR1 |= TIM_CR1_CEN;

	// NVIC
	NVIC_EnableIRQ(TIM4_IRQn);
}

void set_pin_input() {
	// PB8 → Alternate Function (10)
	GPIOB->MODER &= ~(3 << (8 * 2));
	GPIOB->MODER |= (2 << (8 * 2));

	// AF2 (TIM4)
	GPIOB->AFR[1] &= ~(0xF << ((8 - 8) * 4));
	GPIOB->AFR[1] |= (2 << ((8 - 8) * 4));

}

void delay_us(uint32_t us) {
	uint16_t start = TIM4->CNT;
	while ((uint16_t) (TIM4->CNT - start) < us)
		;
}

void set_pin_output_low() {

	// PB8 already output
	GPIOB->BSRR = (1 << (8 + 16)); // LOW
	delay_us(2000);                // ~2 ms

	GPIOB->BSRR = (1 << 8);        // HIGH
	delay_us(30);                  // 20–40 µs

	set_pin_input();
	// reset timer + state
	current_state = WAIT_RESPONSE_LOW;
	memset(data_buff, 0, 5);
	byte_index = 0;
	bit_index = 0;
	capture_prev = 0;
	// Enable interrupt
	TIM4->DIER |= TIM_DIER_CC3IE;

	TIM4->CNT = 0;

}

void TIM4_IRQHandler(void) {

	if (TIM4->SR & TIM_SR_CC3IF) {

		TIM4->SR &= ~TIM_SR_CC3IF;

		uint8_t level = (GPIOB->IDR >> 8) & 1;

		if (level == 1) {
			// Rising edge
			if (current_state == WAIT_RESPONSE_HIGH) {

				current_state = WAIT_BIT_RISE;
			}

			if (current_state == WAIT_BIT_RISE) {
				capture_prev = TIM4->CCR3;
				current_state = WAIT_BIT_FALL;
			}

		} else {
			// Falling edge
			if (current_state == WAIT_RESPONSE_LOW)  //For initial falling edge
				current_state = WAIT_RESPONSE_HIGH;

			if (current_state == WAIT_BIT_FALL) {
				capture_now = TIM4->CCR3;
				delta = capture_now - capture_prev; // no need to look out for overflow because unsigned arithmetic is % 2^16 so accounts for overflow
				bit = (delta > 50) ? 1 : 0; // at 100MHz ticks = (t (seconds) * timer freq)/Prescaler

				if (byte_index < 5) {
					data_buff[byte_index] |= bit << (7 - bit_index);
					bit_index++;
					current_state = WAIT_BIT_RISE;
					if (bit_index >= 8) {
						bit_index = 0;
						byte_index++;
					}

					if (byte_index == 5) {

						uint8_t sum = data_buff[0] + data_buff[1] + data_buff[2]
								+ data_buff[3];
						if ((sum & 0xFF) != data_buff[4]) { //The addition (sum) may go beyond 8 bits so cut off the 9th bit  using & 0xFF
							err_status = CHECKSUM_ERROR;
							memset(data_buff, 0, 5);
						} else {
							err_status = OK;
						}

						current_state = DONE;
						TIM4->DIER &= ~TIM_DIER_CC3IE;

					}
				}

			}

		}

	}
}

dht22_status_t dht22_read(uint8_t *out) {

	set_pin_output_low();  // start measurement
	uint32_t start = xTaskGetTickCount();
	// wait until ISR finishes
	while (current_state != DONE) {
		if ((xTaskGetTickCount() - start) > pdMS_TO_TICKS(10)) {
			err_status = TIMEOUT;
			TIM4->DIER &= ~TIM_DIER_CC3IE;
			current_state = DONE;
			return err_status;
		}
	}

	memcpy(out, data_buff, 5);
	return err_status;
}

