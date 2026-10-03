/**
 * Private configuration file for the SSD1306 library.
 * This example is configured for STM32F0, I2C and including all fonts.
 */

#ifndef __SSD1306_CONF_H__
#define __SSD1306_CONF_H__

// Choose a microcontroller family
//#define STM32F0
//#define STM32F1
#define STM32F4
//#define STM32L0
//#define STM32L1
//#define STM32L4
//#define STM32F3
//#define STM32H7
//#define STM32F7
//#define STM32G0
//#define STM32C0
//#define STM32U5

// Choose a bus
#define SSD1306_USE_I2C
//#define SSD1306_USE_SPI

// I2C Configuration
#define SSD1306_I2C_PORT        hi2c1
#define SSD1306_I2C_ADDR        (0x3C << 1)

// SPI Configuration
//#define SSD1306_SPI_PORT        hspi1
//#define SSD1306_CS_Port         OLED_CS_GPIO_Port
//#define SSD1306_CS_Pin          OLED_CS_Pin
//#define SSD1306_DC_Port         OLED_DC_GPIO_Port
//#define SSD1306_DC_Pin          OLED_DC_Pin
//#define SSD1306_Reset_Port      OLED_Res_GPIO_Port
//#define SSD1306_Reset_Pin       OLED_Res_Pin

// Display controller. The HOPE 1.5" 128x128 OLED is an SH1107, whose init
// sequence differs from the SSD1306 one. Comment this out for an SSD1306/SH1106.
#define SSD1306_USE_SH1107

// SH1107 tuning (only used when SSD1306_USE_SH1107 is defined).
// Display offset: 0x00 for 128x128 panels. If the picture is shifted/wrapped
// vertically (e.g. the top part appears at the bottom), try 0x60 or 0x20.
#define SH1107_DISPLAY_OFFSET   0x00
// DC-DC setting sent with command 0xAD (value used by Adafruit's SH1107
// driver). If the screen stays completely dark, try 0x8B or 0x81.
#define SH1107_DCDC_SETTING     0x8A

// Brightness 0x00-0xFF. 0xFF = maximum; a lower value (e.g. 0x40) is gentler
// in a dark nursery and slows OLED burn-in.
#define SSD1306_CONTRAST        0xFF

// Mirror the screen if needed (for SH1107: HORIZ flips left/right, VERT flips
// up/down; both together = rotate 180 degrees)
// #define SSD1306_MIRROR_VERT
// #define SSD1306_MIRROR_HORIZ

// Set inverse color if needed
// # define SSD1306_INVERSE_COLOR

// Include only needed fonts
#define SSD1306_INCLUDE_FONT_6x8
#define SSD1306_INCLUDE_FONT_7x10
#define SSD1306_INCLUDE_FONT_11x18
#define SSD1306_INCLUDE_FONT_16x26

#define SSD1306_INCLUDE_FONT_16x24

#define SSD1306_INCLUDE_FONT_16x15

// The width of the screen can be set using this
// define. The default value is 128.
#define SSD1306_WIDTH           128

// If your screen horizontal axis does not start
// in column 0 you can use this define to
// adjust the horizontal offset
// #define SSD1306_X_OFFSET

// The height can be changed as well if necessary.
// It can be 32, 64 or 128. The default value is 64.
#define SSD1306_HEIGHT          128

#endif /* __SSD1306_CONF_H__ */
