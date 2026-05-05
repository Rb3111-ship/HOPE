/*
 * ble_driver.c
 *
 *  Created on: 4 May 2026
 *      Author: whp27
 */
#include "ble_driver.h"
#include "main.h"

void BLE_Power_On(void) {
	// Assuming your transistor turns ON when the pin is HIGH (Standard NPN/N-Channel setup)
	HAL_GPIO_WritePin(BLE_SWITCH_GPIO_Port, BLE_SWITCH_Pin, GPIO_PIN_SET);
}

void BLE_Power_Off(void) {
	HAL_GPIO_WritePin(BLE_SWITCH_GPIO_Port, BLE_SWITCH_Pin, GPIO_PIN_RESET);
}
