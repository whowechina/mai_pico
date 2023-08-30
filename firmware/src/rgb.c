/*
 * RGB LED (WS2812) Strip control
 * WHowe <github.com/whowechina>
 * 
 */

#include "rgb.h"

#include "buttons.h"
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

static const uint8_t button_rgb_map[BUTTON_RGB_NUM] = BUTTON_RGB_MAP;

static void trap() {}
static tt_effect_t effects[10] = { {trap, trap, trap, 0} };
static size_t effect_num = 0;
static unsigned current_effect = 0;

#define _MAP_LED(x) _MAKE_MAPPER(x)
#define _MAKE_MAPPER(x) MAP_LED_##x
#define MAP_LED_RGB { c1 = r; c2 = g; c3 = b; }
#define MAP_LED_GRB { c1 = g; c2 = r; c3 = b; }

#define REMAP_BUTTON_RGB _MAP_LED(BUTTON_RGB_ORDER)
#define REMAP_TT_RGB _MAP_LED(TT_RGB_ORDER)

static inline uint32_t _rgb32(uint32_t c1, uint32_t c2, uint32_t c3, bool gamma_fix)
{
    c1 = c1 * iidx_cfg->level / 255;
    c2 = c2 * iidx_cfg->level / 255;
    c3 = c3 * iidx_cfg->level / 255;

    if (gamma_fix) {
        c1 = ((c1 + 1) * (c1 + 1) - 1) >> 8;
        c2 = ((c2 + 1) * (c2 + 1) - 1) >> 8;
        c3 = ((c3 + 1) * (c3 + 1) - 1) >> 8;
    }
    
    return (c1 << 16) | (c2 << 8) | (c3 << 0);    
}

uint32_t button_rgb32(uint32_t r, uint32_t g, uint32_t b, bool gamma_fix)
{
#if BUTTON_RGB_ORDER == GRB
    return _rgb32(g, r, b, gamma_fix);
#else
    return _rgb32(r, g, b, gamma_fix);
#endif
}

uint32_t tt_rgb32(uint32_t r, uint32_t g, uint32_t b, bool gamma_fix)
{
#if TT_RGB_ORDER == GRB
    return _rgb32(g, r, b, gamma_fix);
#else
    return _rgb32(r, g, b, gamma_fix);
#endif
}

uint8_t rgb_button_num()
{
    return BUTTON_RGB_NUM;
}

uint8_t button_lights[BUTTON_RGB_NUM];
uint32_t tt_led_buf[128] = {0};
uint32_t tt_led_angle = 0;

static uint32_t button_led_buf[BUTTON_RGB_NUM] = {0};

void set_effect(uint32_t index)
{
    if (index < effect_num) {
        current_effect = index;
        effects[current_effect].init(effects[current_effect].context);
    } else {
        current_effect = effect_num;
    }
}

void drive_led()
{
    for (int i = 0; i < ARRAY_SIZE(button_led_buf); i++) {
        pio_sm_put_blocking(pio0, 0, button_led_buf[i] << 8u);
    }

    if (iidx_cfg->tt_led.mode == 2) {
        return;
    }

    for (int i = 0; i < iidx_cfg->tt_led.start; i++) {
        pio_sm_put_blocking(pio1, 0, 0);
    }
    for (int i = 0; i < TT_LED_NUM; i++) {
        bool reversed = iidx_cfg->tt_led.mode & 0x01;
        uint8_t id = reversed ? TT_LED_NUM - i - 1 : i;
        pio_sm_put_blocking(pio1, 0, tt_led_buf[id] << 8u);
    }
    for (int i = 0; i < 8; i++) { // a few more to wipe out the last led
        pio_sm_put_blocking(pio1, 0, 0);
    }
}

static uint32_t rgb32_from_hsv(hsv_t hsv)
{
    uint32_t region, remainder, p, q, t;

    if (hsv.s == 0) {
        return hsv.v << 16 | hsv.v << 8 | hsv.v;
    }

    region = hsv.h / 43;
    remainder = (hsv.h % 43) * 6;

    p = (hsv.v * (255 - hsv.s)) >> 8;
    q = (hsv.v * (255 - ((hsv.s * remainder) >> 8))) >> 8;
    t = (hsv.v * (255 - ((hsv.s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0:
            return hsv.v << 16 | t << 8 | p;
        case 1:
            return q << 16 | hsv.v << 8 | p;
        case 2:
            return p << 16 | hsv.v << 8 | t;
        case 3:
            return p << 16 | q << 8 | hsv.v;
        case 4:
            return t << 16 | p << 8 | hsv.v;
        default:
            return hsv.v << 16 | p << 8 | q;
    }
}

uint32_t button_hsv(hsv_t hsv)
{
    uint32_t rgb = rgb32_from_hsv(hsv);
    uint32_t r = (rgb >> 16) & 0xff;
    uint32_t g = (rgb >> 8) & 0xff;
    uint32_t b = (rgb >> 0) & 0xff;
#if BUTTON_RGB_ORDER == GRB
    return _rgb32(g, r, b, false);
#else
    return _rgb32(r, g, b, false);
#endif
}

uint32_t tt_hsv(hsv_t hsv)
{
    uint32_t rgb = rgb32_from_hsv(hsv);
    uint32_t r = (rgb >> 16) & 0xff;
    uint32_t g = (rgb >> 8) & 0xff;
    uint32_t b = (rgb >> 0) & 0xff;
#if TT_RGB_ORDER == GRB
    return _rgb32(g, r, b, false);
#else
    return _rgb32(r, g, b, false);
#endif
}

#define HID_EXPIRE_DURATION 1000000ULL
static uint64_t hid_expire_time = 0;

static void button_lights_update()
{
    for (int i = 0; i < BUTTON_RGB_NUM; i++) {
        int led = button_rgb_map[i];
        if (button_lights[i] > 0) {
            button_led_buf[led] = button_hsv(iidx_cfg->key_on[i]);
        } else {
            button_led_buf[led] = button_hsv(iidx_cfg->key_off[i]);
        }
    }
}

void rgb_set_angle(uint32_t angle)
{
    tt_led_angle = angle;
    effects[current_effect].set_angle(angle);
}

void rgb_set_button_light(uint16_t buttons)
{
    if (time_us_64() < hid_expire_time) {
        return;
    }
    for (int i = 0; i < BUTTON_RGB_NUM; i++) {
        uint16_t flag = 1 << i;
        button_lights[i] = (buttons & flag) > 0 ? 0xff : 0;
    }
}

void rgb_set_hid_light(uint8_t const *lights, uint8_t num)
{
    memcpy(button_lights, lights, num);
    hid_expire_time = time_us_64() + HID_EXPIRE_DURATION;
}

static void effect_update()
{
    effects[current_effect].update(effects[current_effect].context);
}

#define FORCE_EXPIRE_DURATION 100000ULL
static uint64_t force_expire_time = 0;
uint32_t *force_buttons = NULL;
uint32_t *force_tt = NULL;

void force_update()
{
    for (int i = 0; i < BUTTON_RGB_NUM; i++) {
        int led = button_rgb_map[i];
        button_led_buf[led] = force_buttons[i];
    }

    memcpy(tt_led_buf, force_tt, TT_LED_NUM * sizeof(uint32_t));
}

void rgb_force_display(uint32_t *buttons, uint32_t *tt)
{
    force_buttons = buttons;
    force_tt = tt;
    force_expire_time = time_us_64() + FORCE_EXPIRE_DURATION;
}

static void wipe_out_tt_led()
{
    sleep_ms(5);
    for (int i = 0; i < 128; i++) {
        pio_sm_put_blocking(pio1, 0, 0);
    }
    sleep_ms(5);
}

static uint pio1_offset;
static bool pio1_running = false;

static void pio1_run()
{
    gpio_set_drive_strength(TT_RGB_PIN, GPIO_DRIVE_STRENGTH_8MA);
    ws2812_program_init(pio1, 0, pio1_offset, TT_RGB_PIN, 800000, false);
}

static void pio1_stop()
{
    wipe_out_tt_led();
    pio_sm_set_enabled(pio1, 0, false);

    gpio_set_function(TT_RGB_PIN, GPIO_FUNC_SIO);
    gpio_set_dir(TT_RGB_PIN, GPIO_IN);
    gpio_disable_pulls(TT_RGB_PIN);
}

void rgb_init()
{
    uint pio0_offset = pio_add_program(pio0, &ws2812_program);
    pio1_offset = pio_add_program(pio1, &ws2812_program);

    gpio_set_drive_strength(BUTTON_RGB_PIN, GPIO_DRIVE_STRENGTH_2MA);
    ws2812_program_init(pio0, 0, pio0_offset, BUTTON_RGB_PIN, 800000, false);

    /* We don't start the tt LED program yet */
}

static void follow_mode_change()
{
    bool pio1_should_run = (iidx_cfg->tt_led.mode != 2);
    if (pio1_should_run == pio1_running) {
        return;
    }
    pio1_running = pio1_should_run;
    if (pio1_should_run) {
        pio1_run();
    } else {
        pio1_stop();
    }
}

void rgb_update()
{
    follow_mode_change();
    set_effect(iidx_cfg->tt_led.effect);
    if (time_us_64() > force_expire_time) {
        effect_update();
        button_lights_update();
    } else {
        force_update();
    }
    drive_led();
}

void rgb_reg_tt_effect(tt_effect_t effect)
{
    effects[effect_num] = effect;
    effect_num++;
    effects[effect_num] = (tt_effect_t) { trap, trap, trap, 0 };
}
