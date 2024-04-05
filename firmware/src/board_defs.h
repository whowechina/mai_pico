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
#define RGB_BUTTON_MAP { 5, 4, 3, 2, 1, 0, 7, 6, 8, 9, 10, 11 }

/* 8 main buttons, Test, Service, Navigate, Coin */
#define BUTTON_DEF { 1, 0, 4, 5, 8, 9, 3, 2, 12, 11, 10, 14 }

/* HID Keycode: https://github.com/hathach/tinyusb/blob/master/src/class/hid/hid.h */
// P1: WEDCXZAQ3(F1)(F2)(F3) P2: (Numpad)89632147*(F1)(F2)(F3)
#define BUTTON_NKRO_MAP_P1 "\x1a\x08\x07\x06\x1b\x1d\x04\x14\x20\x3a\x3b\x3c"
#define BUTTON_NKRO_MAP_P2 "\x60\x61\x5e\x5b\x5a\x59\x5c\x5f\x55\x3a\x3b\x3c"

#define TOUCH_MAP { E3, A2, B2, D2, E2, A1, B1, D1, E1, C2, A8, B8, \
                    D8, E8, A7, B7, D7, E7, A6, B6, D6, E6, A5, B5, \
                    D5, E5, C1, A4, B4, D4, E4, A3, B3, D3, XX, XX }
#else

#endif
