/*
 * Chu Pico Silder Keys
 * WHowe <github.com/whowechina>
 */

#ifndef Silder_H
#define Silder_H

#include <stdint.h>
#include <stdbool.h>

enum touch_pads {
    A1 = 0, A2, A3, A4, A5, A6, A7, A8,
    B1, B2, B3, B4, B5, B6, B7, B8,
    C1, C2, D1, D2, D3, D4, D5, D6, D7, D8,
    E1, E2, E3, E4, E5, E6, E7, E8,
    XX = 255
};

const char *touch_pad_name(unsigned i);

void touch_init();
void touch_update();
bool touch_touched(unsigned key);
uint64_t touch_touchmap();

const uint16_t *touch_raw();
bool touch_sensor_ok(unsigned i);

void touch_update_config();
unsigned touch_count(unsigned key);
void touch_reset_stat();

#endif
