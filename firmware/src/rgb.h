/*
 * RGB LED (WS2812) Strip control
 * WHowe <github.com/whowechina>
 */

#ifndef RGB_H
#define RGB_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "config.h"

void rgb_init();
void rgb_update();

uint32_t rgb32(uint32_t r, uint32_t g, uint32_t b, bool gamma_fix);
uint32_t rgb32_from_hsv(uint8_t h, uint8_t s, uint8_t v);

void rgb_set_button_color(unsigned index, uint32_t color);
void rgb_set_cab_color(unsigned index, uint32_t color);

/* num of the rgb leds, num*3 bytes in the array */
void rgb_set_brg(unsigned index, const uint8_t *brg_array, size_t num);

#endif
