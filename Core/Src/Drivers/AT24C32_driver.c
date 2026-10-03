/*
 * AT24C32_driver.c
 *
 * 4 KB I2C EEPROM found on most DS3231 RTC modules. It sits on the same I2C1
 * bus as the OLED and the RTC, so every access takes i2c_mutex.
 */
#include "FreeRTOS.h"
#include "semphr.h"
#include "main.h"           // HAL + i2c1_bus_recover()
#include "AT24C32_driver.h"

/* 7-bit address 0x57 = A0..A2 pads open (pulled high), the default on
 * DS3231 modules. If the pads are bridged to ground it becomes 0x50-0x56. */
#define EEPROM_ADDR            (0x57 << 1)
#define EEPROM_IO_TIMEOUT_MS   20U
#define EEPROM_WRITE_CYCLE_MS  10U  // internal write time after each page write
#define EEPROM_MUTEX_TIMEOUT_MS 50U

extern I2C_HandleTypeDef hi2c1;
extern SemaphoreHandle_t i2c_mutex;

static void eeprom_handle_error(HAL_StatusTypeDef status) {
	// A plain NACK (chip absent or busy writing) needs no recovery
	if (!(status == HAL_ERROR && HAL_I2C_GetError(&hi2c1) == HAL_I2C_ERROR_AF)) {
		i2c1_bus_recover();
	}
}

/**
 * @brief Reads len bytes starting at addr. Returns false if the bus or chip failed.
 */
bool eeprom_read(uint16_t addr, uint8_t *data, uint16_t len) {
	if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(EEPROM_MUTEX_TIMEOUT_MS)) != pdPASS) {
		return false;
	}
	HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c1, EEPROM_ADDR, addr,
			I2C_MEMADD_SIZE_16BIT, data, len, EEPROM_IO_TIMEOUT_MS);
	if (status != HAL_OK) {
		eeprom_handle_error(status);
	}
	xSemaphoreGive(i2c_mutex);
	return status == HAL_OK;
}

/**
 * @brief Writes up to one page (32 bytes); [addr, addr+len) must not cross a page boundary.
 * @details Waits for the chip's internal write cycle to finish before returning,
 *          so the next EEPROM access can't be NACKed. Blocks the caller ~10 ms.
 */
bool eeprom_write_page(uint16_t addr, const uint8_t *data, uint16_t len) {
	if (len == 0 || len > AT24C32_PAGE_SIZE
			|| (addr / AT24C32_PAGE_SIZE)
					!= ((addr + len - 1) / AT24C32_PAGE_SIZE)) {
		return false;
	}
	if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(EEPROM_MUTEX_TIMEOUT_MS)) != pdPASS) {
		return false;
	}
	HAL_StatusTypeDef status = HAL_I2C_Mem_Write(&hi2c1, EEPROM_ADDR, addr,
			I2C_MEMADD_SIZE_16BIT, (uint8_t*) data, len, EEPROM_IO_TIMEOUT_MS);
	if (status != HAL_OK) {
		eeprom_handle_error(status);
	}
	xSemaphoreGive(i2c_mutex);

	if (status == HAL_OK) {
		// Let the internal write cycle finish without holding the bus lock
		vTaskDelay(pdMS_TO_TICKS(EEPROM_WRITE_CYCLE_MS));
	}
	return status == HAL_OK;
}
