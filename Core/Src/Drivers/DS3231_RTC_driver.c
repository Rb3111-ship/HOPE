/*
 * DS3231_RTC_driver.c
 *
 * Created on: 28 Apr 2026
 * Author: whp27
 */
#include "FreeRTOS.h"
#include "stm32f4xx_hal.h"
#include "DS3231_RTC_driver.h"
#include <stdint.h>
#include "semphr.h"

/* Physical Hardware Address and Tracking Parameters */
#define ADDRS 0x68
#define START_ADDRS 0x00
#define RTC_I2C_TIMEOUT_MS  10U
extern I2C_HandleTypeDef hi2c1;

/* Dedicated global array sectors handling conversion routines locally */
static uint8_t raw_data[3];
static uint8_t processed_data[3];
extern SemaphoreHandle_t i2c_mutex;

/**
 * @brief Iterates down across array properties translating traditional decimal units into structured BCD syntax blocks.
 */
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

/**
 * @brief Iterates down across array properties untangling hardware BCD patterns back into standard decimal.
 */
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

/**
 * @brief Pulls raw parameters from external chip layers via blocking I2C transactions.
 * @details Applies standard 24-hour register extraction mask adjustments before exporting.
 * @return Static local array reference pointing to decoded values.
 */
uint8_t* get_RTC_Data() {

	// Direct blocking read mapping indices out over the I2C physical bus
	if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(50)) == pdPASS) {
		HAL_I2C_Mem_Read(&hi2c1, ADDRS << 1, START_ADDRS, I2C_MEMADD_SIZE_8BIT,
				raw_data, 3, RTC_I2C_TIMEOUT_MS);
		xSemaphoreGive(i2c_mutex);
	}
	BCDtoDeci();
	processed_data[2] &= 0x3F; // Apply bitwise mask stripping upper status elements out of hour fields
	return processed_data;
}

/**
 * @brief Packs decimal timing variables into BCD formats and flushes them to the physical registers.
 */
void set_RTC_Data(uint8_t *deci_Time) {

	for (int i = 0; i < 3; i++) {
		raw_data[i] = deci_Time[i];
	}

	deciToBCD();
	HAL_I2C_Mem_Write(&hi2c1, ADDRS << 1, START_ADDRS, I2C_MEMADD_SIZE_8BIT,
			processed_data, 3, RTC_I2C_TIMEOUT_MS);
}
