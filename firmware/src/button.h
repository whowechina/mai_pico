/*
 * Mai Controller Buttons
 * WHowe <github.com/whowechina>
 */

#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/flash.h"

void button_init();
uint8_t button_num();
void button_update();
uint16_t button_read();

#endif
