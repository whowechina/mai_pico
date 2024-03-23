/*
 * Chu Pico Silder Keys
 * WHowe <github.com/whowechina>
 */

#ifndef Silder_H
#define Silder_H

#include <stdint.h>
#include <stdbool.h>

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
