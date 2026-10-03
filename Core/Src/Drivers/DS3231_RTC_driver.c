/*
 * DS3231_RTC_driver.c
 *
 * Created on: 28 Apr 2026
 * Author: whp27
 */
#include "FreeRTOS.h"
#include "main.h"           // HAL + i2c1_bus_recover()
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
static void deciToBCD(void) {
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
static void BCDtoDeci(void) {
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
 * @return Static local array reference pointing to decoded values, or NULL if the
 *         bus was busy or the read failed (caller should keep its last good time).
 */
uint8_t* get_RTC_Data(void) {

	// Direct blocking read mapping indices out over the I2C physical bus
	if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(50)) != pdPASS) {
		return NULL;
	}
	HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c1, ADDRS << 1, START_ADDRS,
			I2C_MEMADD_SIZE_8BIT, raw_data, 3, RTC_I2C_TIMEOUT_MS);
	if (status != HAL_OK) {
		// A plain NACK means the module isn't answering; anything else
		// (timeout, stuck BUSY, bus error) gets a bus recovery
		if (!(status == HAL_ERROR
				&& HAL_I2C_GetError(&hi2c1) == HAL_I2C_ERROR_AF)) {
			i2c1_bus_recover();
		}
		xSemaphoreGive(i2c_mutex);
		return NULL;
	}
	xSemaphoreGive(i2c_mutex);

	// Strip the non-time bits while the values are still BCD: hours bit 6 = 12/24 h
	// mode and bit 7 unused; seconds/minutes bit 7 unused
	raw_data[0] &= 0x7F;
	raw_data[1] &= 0x7F;
	raw_data[2] &= 0x3F;
	BCDtoDeci();
	return processed_data;
}

/**
 * @brief Packs decimal timing variables into BCD formats and flushes them to the physical registers.
 * @return true if the RTC acknowledged the write.
 */
bool set_RTC_Data(uint8_t *deci_Time) {

	for (int i = 0; i < 3; i++) {
		raw_data[i] = deci_Time[i];
	}

	deciToBCD();

	if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(50)) != pdPASS) {
		return false;
	}
	HAL_StatusTypeDef status = HAL_I2C_Mem_Write(&hi2c1, ADDRS << 1,
			START_ADDRS, I2C_MEMADD_SIZE_8BIT, processed_data, 3,
			RTC_I2C_TIMEOUT_MS);
	if (status != HAL_OK
			&& !(status == HAL_ERROR
					&& HAL_I2C_GetError(&hi2c1) == HAL_I2C_ERROR_AF)) {
		i2c1_bus_recover();
	}
	xSemaphoreGive(i2c_mutex);
	return status == HAL_OK;
}
