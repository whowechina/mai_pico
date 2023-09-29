/*
 * Mai Controller Board Definitions
 * WHowe <github.com/whowechina>
 */

#if defined BOARD_MAI_PICO

#define I2C_PORT i2c1
#define I2C_SDA 6
#define I2C_SCL 7
#define I2C_FREQ 400*1000

#define RGB_PIN 13
#define RGB_ORDER GRB // or RGB

#define NKRO_KEYMAP "1aqz2swx3dec4frv5gtb6hyn7jum8ki90olp,."
#else

#endif
