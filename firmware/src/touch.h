/*
 * Mai Pico Touch Keys
 * WHowe <github.com/whowechina>
 */

#ifndef TOUCH_H
#define TOUCH_H

#include <stdint.h>
#include <stdbool.h>

void touch_init();
void touch_update();
bool touch_touched(unsigned key);
void touch_update_config();

#endif
