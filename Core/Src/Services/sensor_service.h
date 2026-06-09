/*
 * sensor_service.h
 *
 * Created on: 28 Apr 2026
 * Author: whp27
 */

#ifndef SRC_SERVICES_SENSOR_SERVICE_H_
#define SRC_SERVICES_SENSOR_SERVICE_H_
#include <stdint.h>

#include "DHT22_driver.h"

// Service Layer Application Programming Interface Prototypes
void get_sensor_data(float *sensor_data);

#endif /* SRC_SERVICES_SENSOR_SERVICE_H_ */
