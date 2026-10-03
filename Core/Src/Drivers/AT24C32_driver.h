/*
 * AT24C32_driver.h
 *
 * 4 KB I2C EEPROM found on most DS3231 RTC modules (shares the I2C1 bus).
 */

#ifndef SRC_DRIVERS_AT24C32_DRIVER_H_
#define SRC_DRIVERS_AT24C32_DRIVER_H_

#include <stdint.h>
#include <stdbool.h>

#define AT24C32_PAGE_SIZE 32U   // a single write must not cross a 32-byte page boundary
#define AT24C32_SIZE      4096U

// Low-Level Hardware Driver Application Programming Interface Prototypes
bool eeprom_read(uint16_t addr, uint8_t *data, uint16_t len);
bool eeprom_write_page(uint16_t addr, const uint8_t *data, uint16_t len);

#endif /* SRC_DRIVERS_AT24C32_DRIVER_H_ */
