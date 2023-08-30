/*
 * Controller Setup
 * WHowe <github.com/whowechina>
 */

#ifndef SETUP_H
#define SETUP_H

#include <stdint.h>
#include <stdbool.h>
#include "board_defs.h"

void setup_init();
bool setup_run(uint16_t key_flag, uint16_t tt_angle);

extern uint32_t setup_led_tt[];
extern uint32_t setup_led_button[BUTTON_RGB_NUM];

#endif
