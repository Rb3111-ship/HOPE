/*
 * sensor_service.c
 *
 * Fetches humidity and temperature from the DHT22 (called every 5 s by the UI renderer)
 *
 * Created on: 21 Apr 2026
 * Author: whp27
 */
#include "sensor_service.h"
#include <string.h>
#include <stdint.h>

// Global tracking references evaluating error and raw parameters
dht22_status_t sensor_status;

/* Last successful reading, reused when a read fails so the screen doesn't flash 0 */
static float last_humidity = 0.0f;
static float last_temperature = 0.0f;
static bool have_reading = false;

//Byte 0: Humidity high
//Byte 1: Humidity low
//Byte 2: Temperature high
//Byte 3: Temperature low
//Byte 4: Checksum
/**
 * @brief Coordinates low-level driver stream collection and scales metrics to floating points.
 * @details Decodes raw composite bytes, evaluates signed sign-bit shifts, and outputs conversions.
 * @param sensor_data Array destination where indexes 0 (Hum) and 1 (Temp) are saved.
 * @return true if sensor_data holds a real reading (this one or the last good one),
 *         false if the sensor has never been read successfully.
 */
bool get_sensor_data(float *sensor_data)
{
    uint8_t raw[5];

    // Read the un-marshaled byte array directly from physical lines
    sensor_status = dht22_read(raw);

    // On a failed read keep showing the last good values
    if (sensor_status != OK) {
        sensor_data[0] = last_humidity;
        sensor_data[1] = last_temperature;
        return have_reading;
    }

    // Assemble individual byte fields into 16-bit parameters
    uint16_t raw_humidity = (raw[0] << 8) | raw[1];
    uint16_t raw_temp     = (raw[2] << 8) | raw[3];

    float humidity = raw_humidity / 10.0f;

    // Evaluate the absolute sign bit tracking negative temperatures
    float temperature;
    if (raw_temp & 0x8000) {
        raw_temp &= 0x7FFF;
        temperature = -(raw_temp / 10.0f);
    } else {
        temperature = raw_temp / 10.0f;
    }

    // Export conversion elements to output structures
    last_humidity = humidity;
    last_temperature = temperature;
    have_reading = true;
    sensor_data[0] = humidity;
    sensor_data[1] = temperature;
    return true;
}
