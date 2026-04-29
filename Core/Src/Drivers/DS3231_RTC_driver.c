/*
 * DS3231_RTC_driver.c
 *
 *  Created on: 28 Apr 2026
 *      Author: whp27
 */
#include "FreeRTOS.h"
#include "stm32f4xx_hal.h"
#include "DS3231_RTC_driver.h"

#define ADDRS 0x68
#define START_ADDRS 0x00
extern I2C_HandleTypeDef hi2c1;
#include <stdint.h>

uint8_t raw_data[3];
uint8_t processed_data[3];
void deciToBCD() {
	for (int i = 0; i < 3; i++) {
		uint8_t shift = 0;
		uint8_t deci = raw_data[i];
		uint8_t buff = 0;
		uint8_t bcd = 0;
		while (deci != 0) {
			buff = deci % 10;
			bcd |= buff << shift;
			shift += 4;
			deci /= 10;
		}
		processed_data[i] = bcd;
	}
}

void BCDtoDeci() {
	for (int i = 0; i < 3; i++) {
		uint8_t bcd = raw_data[i];
		uint8_t unit = 1;
		uint8_t buff = 0;
		uint8_t deci = 0;
		while (bcd != 0) {
			buff = bcd & 0xF;
			deci += buff * unit;
			unit *= 10;
			bcd >>= 4;
		}
		processed_data[i] = deci;
	}
}

uint32_t* get_RTC_Data() {

	HAL_I2C_Mem_Read(&hi2c1, ADDRS << 1, START_ADDRS, I2C_MEMADD_SIZE_8BIT,
			raw_data, 3, pdMS_TO_TICKS(10));
	BCDtoDeci();
	processed_data[2] &= 0x3F;
	return processed_data;
}

void set_RTC_Data(uint8_t *deci_Time) {

	for (int i = 0; i < 3; i++) {
		raw_data[i] = deci_Time[i];
	}

	deciToBCD();
	HAL_I2C_Mem_Write(&hi2c1, ADDRS << 1, START_ADDRS, I2C_MEMADD_SIZE_8BIT,
			processed_data, 3, pdMS_TO_TICKS(10));

}

