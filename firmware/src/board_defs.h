/*
 * IIDX Controller Board Definitions
 * WHowe <github.com/whowechina>
 */

#if defined BOARD_IIDX_PICO
/* List of button pins */
#define BUTTON_DEF { 8, 7, 6, 5, 4, 3, 2, 12, 11, 10, 9, 1, 0 }

#define BUTTON_RGB_PIN 13
#define BUTTON_RGB_ORDER GRB // or RGB

#define BUTTON_RGB_NUM 11
#define BUTTON_RGB_MAP { 6, 0, 5, 1, 4, 2, 3, 7, 8, 9, 10}


#define TT_RGB_PIN 28
#define TT_RGB_ORDER GRB // or RGB

#define TT_AS5600_ANALOG 26
#define TT_AS5600_SCL 27
#define TT_AS5600_SDA 26
#define TT_AS5600_I2C i2c1

// Alternative I2C pins
//#define TT_AS5600_SCL 21
//#define TT_AS5600_SDA 20
//#define TT_AS5600_I2C i2c0

#else

#endif
