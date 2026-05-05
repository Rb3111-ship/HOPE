/*
 * sensor_service.c
 *
 * Used to run a software timer to fetch humidity and temperature  data from the dht11
 *
 *  Created on: 21 Apr 2026
 *      Author: whp27
 */
#include "sensor_service.h"
#include <string.h>
#include <stdint.h>

//DEAL WITH ERROR
dht22_status_t sensor_status;
uint16_t raw_humidity  = 0;
uint16_t raw_temp  = 0;

//Byte 0: Humidity high
//Byte 1: Humidity low
//Byte 2: Temperature high
//Byte 3: Temperature low
//Byte 4: Checksum
void get_sensor_data(float *sensor_data)
{
    uint8_t raw[5];

    sensor_status = dht22_read(raw);

    if (sensor_status != OK) {
        sensor_data[0] = 0.0f;
        sensor_data[1] = 0.0f;
        return;
    }

    uint16_t raw_humidity = (raw[0] << 8) | raw[1];
    uint16_t raw_temp     = (raw[2] << 8) | raw[3];

    float humidity = raw_humidity / 10.0f;

    float temperature;
    if (raw_temp & 0x8000) {
        raw_temp &= 0x7FFF;
        temperature = -(raw_temp / 10.0f);
    } else {
        temperature = raw_temp / 10.0f;
    }

    sensor_data[0] = humidity;
    sensor_data[1] = temperature;
}
