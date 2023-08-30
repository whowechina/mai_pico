/*
 * RGB LED (WS2812) Strip control
 * WHowe <github.com/whowechina>
 */

#ifndef RGB_H
#define RGB_H

#include <stdint.h>
#include <stdbool.h>

#include "config.h"

void rgb_init();
void rgb_set_hardware(uint16_t tt_start, uint16_t tt_num, bool tt_reversed);

uint8_t rgb_button_num();
void rgb_update();

void rgb_set_angle(uint32_t angle);
void rgb_set_level(uint8_t level);

void rgb_set_button_light(uint16_t buttons);
void rgb_set_hid_light(uint8_t const *lights, uint8_t num);

void rgb_force_display(uint32_t *keyboard, uint32_t *tt);

typedef struct {
    void (*init)(uint32_t context);
    void (*set_angle)(uint32_t angle);
    void (*update)(uint32_t context);
    uint32_t context;
} tt_effect_t;

void rgb_reg_tt_effect(tt_effect_t effect);

extern uint32_t tt_led_buf[];
#define TT_LED_NUM (iidx_cfg->tt_led.num)

/* These global variables meant to be accessed by effect codes */
extern uint32_t tt_led_angle;


uint32_t button_rgb32(uint32_t r, uint32_t g, uint32_t b, bool gamma_fix);
uint32_t tt_rgb32(uint32_t r, uint32_t g, uint32_t b, bool gamma_fix);
uint32_t button_hsv(hsv_t hsv);
uint32_t tt_hsv(hsv_t hsv);

#endif
