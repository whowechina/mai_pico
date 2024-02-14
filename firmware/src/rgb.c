/*
 * RGB LED (WS2812) Strip control
 * WHowe <github.com/whowechina>
 * 
 */

#include "rgb.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "bsp/board.h"
#include "hardware/pio.h"
#include "hardware/timer.h"

#include "ws2812.pio.h"

#include "board_defs.h"
#include "config.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static struct {
    uint32_t color;
    uint32_t target; // target color
    uint16_t duration;
    uint16_t elapsed;
} rgb_ctrl[20];
static const uint8_t button_led_map[] = RGB_BUTTON_MAP;

#define _MAP_LED(x) _MAKE_MAPPER(x)
#define _MAKE_MAPPER(x) MAP_LED_##x
#define MAP_LED_RGB { c1 = r; c2 = g; c3 = b; }
#define MAP_LED_GRB { c1 = g; c2 = r; c3 = b; }

#define REMAP_BUTTON_RGB _MAP_LED(BUTTON_RGB_ORDER)
#define REMAP_TT_RGB _MAP_LED(TT_RGB_ORDER)

static inline uint32_t _rgb32(uint32_t c1, uint32_t c2, uint32_t c3, bool gamma_fix)
{
    if (gamma_fix) {
        c1 = ((c1 + 1) * (c1 + 1) - 1) >> 8;
        c2 = ((c2 + 1) * (c2 + 1) - 1) >> 8;
        c3 = ((c3 + 1) * (c3 + 1) - 1) >> 8;
    }
    
    return (c1 << 16) | (c2 << 8) | (c3 << 0);    
}

uint32_t rgb32(uint32_t r, uint32_t g, uint32_t b, bool gamma_fix)
{
#if BUTTON_RGB_ORDER == GRB
    return _rgb32(g, r, b, gamma_fix);
#else
    return _rgb32(r, g, b, gamma_fix);
#endif
}

uint32_t gray32(uint32_t c, bool gamma_fix)
{
    return rgb32(c, c, c, gamma_fix);
}

uint32_t rgb32_from_hsv(uint8_t h, uint8_t s, uint8_t v)
{
    uint32_t region, remainder, p, q, t;

    if (s == 0) {
        return v << 16 | v << 8 | v;
    }

    region = h / 43;
    remainder = (h % 43) * 6;

    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0:
            return v << 16 | t << 8 | p;
        case 1:
            return q << 16 | v << 8 | p;
        case 2:
            return p << 16 | v << 8 | t;
        case 3:
            return p << 16 | q << 8 | v;
        case 4:
            return t << 16 | p << 8 | v;
        default:
            return v << 16 | p << 8 | q;
    }
}

static uint32_t lerp(uint32_t a, uint32_t b, uint8_t t)
{
    uint32_t c1 = ((a & 0xff0000) * (255 - t) + (b & 0xff0000) * t) & 0xff000000;
    uint32_t c2 = ((a & 0xff00) * (255 - t) + (b & 0xff00) * t) & 0xff0000;
    uint32_t c3 = ((a & 0xff) * (255 - t) + (b & 0xff) * t) & 0xff00;
    return c1 | c2 | c3;
}

static void drive_led()
{
    static uint64_t last = 0;
    uint64_t now = time_us_64();
    if (now - last < 4000) { // no faster than 250Hz
        return;
    }
    last = now;

    for (int i = 0; i < ARRAY_SIZE(rgb_ctrl); i++) {
        int num = (i < 8) ? mai_cfg->rgb.per_button : mai_cfg->rgb.per_aux;
        for (int j = 0; j < num; j++) {
            pio_sm_put_blocking(pio0, 0, rgb_ctrl[i].color << 8u);
        }
    }
}

static void fade_ctrl()
{
    static uint64_t last = 0;
    uint64_t now = time_us_64();
    uint64_t delta = now - last;

    for (int i = 0; i < ARRAY_SIZE(rgb_ctrl); i++) {
        if (rgb_ctrl[i].duration == 0) {
            continue;
        }

        rgb_ctrl[i].elapsed += delta;
        if (rgb_ctrl[i].elapsed >= rgb_ctrl[i].duration) {
            rgb_ctrl[i].color = rgb_ctrl[i].target;
            rgb_ctrl[i].duration = 0;
            continue;
        }

        uint8_t progress = rgb_ctrl[i].elapsed * 255 / rgb_ctrl[i].duration;
        rgb_ctrl->color = lerp(rgb_ctrl->color, rgb_ctrl->target, progress);
    }

    last = now;
}

static inline uint32_t apply_level(uint32_t color)
{
    unsigned r = (color >> 16) & 0xff;
    unsigned g = (color >> 8) & 0xff;
    unsigned b = color & 0xff;

    r = r * mai_cfg->color.level / 255;
    g = g * mai_cfg->color.level / 255;
    b = b * mai_cfg->color.level / 255;

    return r << 16 | g << 8 | b;
}

static void set_color(unsigned index, uint32_t color, uint8_t speed)
{
    if (index >= ARRAY_SIZE(rgb_ctrl)) {
        return;
    }

    if (speed > 0) {
        rgb_ctrl[index].target = apply_level(color);
        rgb_ctrl[index].duration = 32767 / speed;
        rgb_ctrl[index].elapsed = 0;
    } else {
        rgb_ctrl[index].color = apply_level(color);
        rgb_ctrl[index].duration = 0;
    }
}

void rgb_set_button(unsigned index, uint32_t color, uint8_t speed)
{
    if (index >= ARRAY_SIZE(button_led_map)) {
        return;
    }
    set_color(button_led_map[index], color, speed);
}

void rgb_set_cab(unsigned index, uint32_t color)
{
    if (index >= 3) {
        return;
    }
    set_color(8 + index, color, 0);
}

void rgb_init()
{
    uint pio0_offset = pio_add_program(pio0, &ws2812_program);

    gpio_set_drive_strength(RGB_PIN, GPIO_DRIVE_STRENGTH_2MA);
    ws2812_program_init(pio0, 0, pio0_offset, RGB_PIN, 800000, false);
}

void rgb_update()
{
    fade_ctrl();
    drive_led();
}
