/*
 * sensor_service.h
 *
 * Created on: 28 Apr 2026
 * Author: whp27
 */

#ifndef SRC_SERVICES_SENSOR_SERVICE_H_
#define SRC_SERVICES_SENSOR_SERVICE_H_
#include <stdint.h>
#include <stdbool.h>

#include "DHT22_driver.h"

// Service Layer Application Programming Interface Prototypes
bool get_sensor_data(float *sensor_data); // false until the first good reading

#endif /* SRC_SERVICES_SENSOR_SERVICE_H_ */
