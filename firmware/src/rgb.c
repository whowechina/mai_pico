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

static uint32_t rgb_buf[47]; // 16(Keys) + 15(Gaps) + 16(maximum ToF indicators)

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

static void drive_led()
{
    static uint64_t last = 0;
    uint64_t now = time_us_64();
    if (now - last < 4000) { // no faster than 250Hz
        return;
    }
    last = now;

    for (int i = 30; i >= 0; i--) {
        pio_sm_put_blocking(pio0, 0, rgb_buf[i] << 8u);
    }
    for (int i = 31; i < ARRAY_SIZE(rgb_buf); i++) {
        pio_sm_put_blocking(pio0, 0, rgb_buf[i] << 8u);
    }
}

void rgb_set_colors(const uint32_t *colors, unsigned index, size_t num)
{
    if (index >= ARRAY_SIZE(rgb_buf)) {
        return;
    }
    if (index + num > ARRAY_SIZE(rgb_buf)) {
        num = ARRAY_SIZE(rgb_buf) - index;
    }
    memcpy(&rgb_buf[index], colors, num * sizeof(*colors));
}

void rgb_set_color(unsigned index, uint32_t color)
{
    if (index >= ARRAY_SIZE(rgb_buf)) {
        return;
    }
    rgb_buf[index] = color;
}

void rgb_key_color(unsigned index, uint32_t color)
{
    if (index > 16) {
        return;
    }
    rgb_buf[index * 2] = color;
}

void rgb_gap_color(unsigned index, uint32_t color)
{
    if (index > 15) {
        return;
    }
    rgb_buf[index * 2 + 1] = color;
}

void rgb_set_brg(unsigned index, const uint8_t *brg_array, size_t num)
{
    if (index >= ARRAY_SIZE(rgb_buf)) {
        return;
    }
    if (index + num > ARRAY_SIZE(rgb_buf)) {
        num = ARRAY_SIZE(rgb_buf) - index;
    }
    for (int i = 0; i < num; i++) {
        uint8_t b = brg_array[i * 3 + 0];
        uint8_t r = brg_array[i * 3 + 1];
        uint8_t g = brg_array[i * 3 + 2];
        rgb_buf[index + i] = rgb32(r, g, b, false);
    }
}


void rgb_init()
{
    uint pio0_offset = pio_add_program(pio0, &ws2812_program);

    gpio_set_drive_strength(RGB_PIN, GPIO_DRIVE_STRENGTH_2MA);
    ws2812_program_init(pio0, 0, pio0_offset, RGB_PIN, 800000, false);
}

void rgb_update()
{
    drive_led();
}
