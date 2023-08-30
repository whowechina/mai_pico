/*
 * Controller Setup Menu
 * WHowe <github.com/whowechina>
 * 
 * Setup is a mode, so one can change settings live
 */

#include "setup.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "bsp/board.h"
#include "pico/bootrom.h"

#include "rgb.h"
#include "config.h"

static iidx_cfg_t cfg_save;

static uint64_t setup_tick_ms = 0;
#define CONCAT(a, b) a ## b
#define TVAR(line) CONCAT(a, line)
#define RUN_EVERY_N_MS(a, ms) { static uint64_t TVAR(__LINE__) = 0; \
    if (setup_tick_ms - TVAR(__LINE__) >= ms) { a; TVAR(__LINE__) = setup_tick_ms; } }
static uint32_t blink_fast = 0xffffffff;
static uint32_t blink_slow = 0xffffffff;

uint32_t setup_led_tt[128];
uint32_t setup_led_button[BUTTON_RGB_NUM];

typedef enum {
    MODE_NONE,
    MODE_TURNTABLE,
    MODE_ANALOG,
    MODE_LEVEL,
    MODE_TT_THEME,
    MODE_KEY_THEME,
    MODE_KEY_OFF,
    MODE_KEY_ON,
} setup_mode_t;
static setup_mode_t current_mode = MODE_NONE;

static struct {
    uint16_t last_keys;
    uint16_t keys;
    uint16_t just_pressed;
    uint16_t just_released;

    int16_t last_angle;
    int16_t angle;
    int16_t rotate;
} input = { 0 };

#define KEY_1 0x0001
#define KEY_2 0x0002
#define KEY_3 0x0004      
#define KEY_4 0x0008 
#define KEY_5 0x0010 
#define KEY_6 0x0020
#define KEY_7 0x0040 
#define E1    0x0080 
#define E2    0x0100 
#define E3    0x0200
#define E4    0x0400 
#define AUX_NO  0x0800 
#define AUX_YES 0x1000

#define LED_KEY_1 0
#define LED_KEY_2 1
#define LED_KEY_3 2
#define LED_KEY_4 3
#define LED_KEY_5 4
#define LED_KEY_6 5
#define LED_KEY_7 6
#define LED_E1 7
#define LED_E2 8
#define LED_E3 9
#define LED_E4 10

#define PRESSED_ALL(k) ((input.keys & (k)) == (k))
#define PRESSED_ANY(k) (input.keys & (k))
#define JUST_PRESSED(k) (input.just_pressed & (k))
#define JUST_RELEASED(k) (input.just_released & (k))

#define RED button_rgb32(99, 0, 0, false)
#define GREEN button_rgb32(0, 99, 0, false)
#define CYAN button_rgb32(0, 40, 99, false)
#define YELLOW button_rgb32(99, 99, 0, false)
#define SILVER button_rgb32(60, 60, 60, false)

#define TT_RED tt_rgb32(99, 0, 0, false)
#define TT_GREEN tt_rgb32(0, 99, 0, false)
#define TT_CYAN tt_rgb32(0, 40, 99, false)
#define TT_YELLOW tt_rgb32(99, 99, 0, false)
#define TT_SILVER tt_rgb32(60, 60, 60, false)

typedef void (*mode_func)();

static void join_mode(setup_mode_t new_mode);
static void quit_mode(bool apply);

static void nop()
{
}

static void check_exit()
{
    if (JUST_PRESSED(AUX_YES)) {
        quit_mode(true);
    } else if (JUST_PRESSED(AUX_NO)) {
        quit_mode(false);
    }
}

static int16_t input_delta(int16_t start_angle)
{
    int16_t delta = input.angle - start_angle;
    if (delta > 128) {
        delta -= 256;
    }
    if (delta < -128) {
        delta += 256;
    }
    return delta;
}

static setup_mode_t key_to_mode[11] = {
    MODE_KEY_THEME, MODE_TT_THEME, MODE_KEY_ON, MODE_KEY_OFF,
    MODE_NONE, MODE_NONE, MODE_NONE,
    MODE_ANALOG, MODE_ANALOG, MODE_ANALOG, MODE_ANALOG,
};

static struct {
    bool escaped;
    uint64_t escape_time;
    uint16_t start_angle;
} none_ctx = { 0 };

static void none_rotate()
{
    if (!none_ctx.escaped) {
        return;
    }

    int16_t delta = input_delta(none_ctx.start_angle);
    if (abs(delta) > 10) {
        join_mode(MODE_LEVEL);
        none_ctx.escaped = false;
    }
}

static void none_loop()
{
    if (PRESSED_ALL(AUX_YES | AUX_NO)) {
        if (!none_ctx.escaped) {
            none_ctx.escaped = true;
            none_ctx.escape_time = time_us_64();
            none_ctx.start_angle = input.angle;
        }
    } else {
        none_ctx.escaped = false;
    }

    if (!none_ctx.escaped) {
        return;
    }

    for (int i = 0; i < 11; i++) {
        if (PRESSED_ANY(KEY_1 << i)) {
            none_ctx.escaped = false;
            join_mode(key_to_mode[i]);
            return;
        }
    }

    if (time_us_64() - none_ctx.escape_time > 5000000) {
        none_ctx.escaped = false;
        join_mode(MODE_TURNTABLE);
        return;
    }
}

static struct {
    uint8_t adjust_led; /* 0: nothing, 1: adjust start, 2: adjust stop */
    int16_t start_angle;
} tt_ctx;

static void tt_enter()
{
    tt_ctx.start_angle = input.angle;
}

static void tt_key_change()
{
    if (JUST_PRESSED(E1)) {
        tt_ctx.adjust_led = (tt_ctx.adjust_led == 1) ? 0 : 1;
        tt_ctx.start_angle = input.angle;
    } else if (JUST_PRESSED(E2)) {
        tt_ctx.adjust_led = (tt_ctx.adjust_led == 2) ? 0 : 2;
        tt_ctx.start_angle = input.angle;
    } else if (JUST_PRESSED(E3)) {
        iidx_cfg->tt_led.mode = (iidx_cfg->tt_led.mode + 1) % 3;
    } else if (JUST_PRESSED(E4)) {
        iidx_cfg->tt_sensor.mode = (iidx_cfg->tt_sensor.mode + 1) % 4;
    } else if (JUST_PRESSED(KEY_2)) {
        iidx_cfg->tt_sensor.deadzone = 0;
    } else if (JUST_PRESSED(KEY_4)) {
        iidx_cfg->tt_sensor.deadzone = 1;
    } else if (JUST_PRESSED(KEY_6)) {
        iidx_cfg->tt_sensor.deadzone = 2;
    } else if (JUST_PRESSED(KEY_1)) {
        iidx_cfg->tt_sensor.ppr = 0;
    } else if (JUST_PRESSED(KEY_3)) {
        iidx_cfg->tt_sensor.ppr = 1;
    } else if (JUST_PRESSED(KEY_5)) {
        iidx_cfg->tt_sensor.ppr = 2;
    } else if (JUST_PRESSED(KEY_7)) {
        iidx_cfg->tt_sensor.ppr = 3;
    }

    check_exit();
}

static void tt_rotate()
{
    int16_t delta = input_delta(tt_ctx.start_angle);
    if (abs(delta) > 8) {
        tt_ctx.start_angle = input.angle;

        #define LED_START iidx_cfg->tt_led.start
        #define LED_NUM iidx_cfg->tt_led.num

        if (tt_ctx.adjust_led == 1) {
            if ((delta > 0) & (LED_START < 8)) {
                LED_START++;
                if (LED_NUM > 1) {
                    LED_NUM--;
                }
            } else if ((delta < 0) & (LED_START > 0)) {
                LED_START--;
                LED_NUM++;
            }
        } else if (tt_ctx.adjust_led == 2) {
            if ((delta > 0) & (LED_NUM + LED_START < 128)) {
                LED_NUM++;
            } else if ((delta < 0) & (LED_NUM > 1)) { // at least 1 led
                LED_NUM--;
            }
        }
    }
}

static void tt_loop()
{
    for (int i = 1; i < iidx_cfg->tt_led.num - 1; i++) {
        setup_led_tt[i] = tt_rgb32(10, 10, 10, false);
    }

    bool led_reversed = (iidx_cfg->tt_led.mode == 1);
    int head = led_reversed ? TT_LED_NUM - 1 : 0;
    int tail = led_reversed ? 0 : TT_LED_NUM - 1;

    setup_led_tt[head] = tt_rgb32(0xa0, 0, 0, false);
    setup_led_tt[tail] = tt_rgb32(0, 0xa0, 0, false);
    setup_led_button[LED_E2] = button_rgb32(0, 10, 0, false);
    setup_led_button[LED_E1] = button_rgb32(10, 0, 0, false);

    if (tt_ctx.adjust_led == 1) {
        setup_led_tt[head] &= blink_fast;
        setup_led_button[LED_E1] = RED & blink_fast;
    } else if (tt_ctx.adjust_led == 2) {
        setup_led_tt[tail] &= blink_fast;
        setup_led_button[LED_E2] = GREEN & blink_fast;
    }

    switch (iidx_cfg->tt_led.mode) {
        case 0:
            setup_led_button[LED_E3] = GREEN;
            break;
        case 1:
            setup_led_button[LED_E3] = RED;
            break;
        default:
            setup_led_button[LED_E3] = 0;
            break;
    }

    switch (iidx_cfg->tt_sensor.mode) {
        case 0:
            setup_led_button[LED_E4] = GREEN;
            break;
        case 1:
            setup_led_button[LED_E4] = RED;
            break;
        case 2:
            setup_led_button[LED_E4] = CYAN;
            break;
        default:
            setup_led_button[LED_E4] = YELLOW;
            break;
    }
    setup_led_button[LED_KEY_2] = iidx_cfg->tt_sensor.deadzone == 0 ? SILVER : 0;
    setup_led_button[LED_KEY_4] = iidx_cfg->tt_sensor.deadzone == 1 ? SILVER : 0;
    setup_led_button[LED_KEY_6] = iidx_cfg->tt_sensor.deadzone == 2 ? SILVER : 0;
    setup_led_button[LED_KEY_1] = iidx_cfg->tt_sensor.ppr == 0 ? SILVER : 0;
    setup_led_button[LED_KEY_3] = iidx_cfg->tt_sensor.ppr == 1 ? SILVER : 0;
    setup_led_button[LED_KEY_5] = iidx_cfg->tt_sensor.ppr == 2 ? SILVER : 0;
    setup_led_button[LED_KEY_7] = iidx_cfg->tt_sensor.ppr == 3 ? SILVER : 0;
}

static void level_rotate()
{
    int16_t new_value = iidx_cfg->level;
    new_value += input.rotate;
    if (new_value < 0) {
        new_value = 0;
    } else if (new_value > 255) {
        new_value = 255;
    }
    iidx_cfg->level = new_value;
    printf("Level: %d\n", iidx_cfg->level);
}

static void level_key_change()
{
    if (JUST_PRESSED(KEY_1)) {
        iidx_cfg->level = 0;
    } else if (JUST_PRESSED(KEY_2)) {
        iidx_cfg->level = 20;
    } else if (JUST_PRESSED(KEY_3)) {
        iidx_cfg->level = 50;
    } else if (JUST_PRESSED(KEY_4)) {
        iidx_cfg->level = 85;
    } else if (JUST_PRESSED(KEY_5)) {
        iidx_cfg->level = 130;
    } else if (JUST_PRESSED(KEY_6)) {
        iidx_cfg->level = 190;
    } else if (JUST_PRESSED(KEY_7)) {
        iidx_cfg->level = 255;
    }

    check_exit();
}

static void level_loop()
{
    for (int i = 0; i < 7; i++) {
        hsv_t key_color = {i * 255 / 7, 255, 255 };
        setup_led_button[i] = button_hsv(key_color);
    }

    uint16_t pos = iidx_cfg->level * iidx_cfg->tt_led.num / 256;
    for (unsigned i = 0; i < iidx_cfg->tt_led.num; i++) {
        setup_led_tt[i] = (i == pos) ? tt_rgb32(90, 90, 90, false) : 0;
    }
}

static struct {
    uint8_t channel; /* 0:E1(Start), 1:E2(Effect), 2:E3(VEFX), 3:E4 */
    volatile uint8_t *value;
    int16_t start_angle;
} analog_ctx;

static void analog_key_change()
{
    if (JUST_PRESSED(E1)) {
        analog_ctx.channel = 0;
        analog_ctx.value = &iidx_cfg->effects.e1;
    } else if (JUST_PRESSED(E2)) {
        analog_ctx.channel = 1;
        analog_ctx.value = &iidx_cfg->effects.e2;
    } else if (JUST_PRESSED(E3)) {
        analog_ctx.channel = 2;
        analog_ctx.value = &iidx_cfg->effects.e3;
    } else if (JUST_PRESSED(E4)) {
        analog_ctx.channel = 3;
        analog_ctx.value = &iidx_cfg->effects.e4;
    } else if (JUST_PRESSED(KEY_1)) {
        *analog_ctx.value = 0;
    } else if (JUST_PRESSED(KEY_2)) {
        *analog_ctx.value = 43;
    } else if (JUST_PRESSED(KEY_3)) {
        *analog_ctx.value = 85;
    } else if (JUST_PRESSED(KEY_4)) {
        *analog_ctx.value = 128;
    } else if (JUST_PRESSED(KEY_5)) {
        *analog_ctx.value = 170;
    } else if (JUST_PRESSED(KEY_6)) {
        *analog_ctx.value = 213;
    } else if (JUST_PRESSED(KEY_7)) {
        *analog_ctx.value = 255;
    }

    check_exit();
}

static void analog_enter()
{
    analog_key_change();
}

static void analog_rotate()
{
    int16_t new_value = *analog_ctx.value;
    new_value += input.rotate;
    if (new_value < 0) {
        new_value = 0;
    } else if (new_value > 255) {
        new_value = 255;
    }
    *analog_ctx.value = new_value;
}

static uint32_t scale_color(uint32_t color, uint8_t value, uint8_t factor)
{
    uint8_t r = (color >> 16) & 0xff;
    uint8_t g = (color >> 8) & 0xff;
    uint8_t b = color & 0xff;

    r = (r * value) / factor;
    g = (g * value) / factor;
    b = (b * value) / factor;

    return (r << 16) | (g << 8) | b;
}

static void analog_loop()
{
    uint32_t colors[4] = { RED, GREEN, CYAN, YELLOW};
    uint32_t tt_colors[4] = { TT_RED, TT_GREEN, TT_CYAN, TT_YELLOW };

    for (int i = 0; i < 4; i++) {
        uint32_t color = colors[i];
        if (analog_ctx.channel == i) {
            color &= blink_fast;
        }
        setup_led_button[LED_E1 + i] = color;
    }

    int tt_split = (int)*analog_ctx.value * iidx_cfg->tt_led.num / 255;

    for (int i = 1; i < iidx_cfg->tt_led.num - 1; i++) {
        setup_led_tt[i] = i < tt_split ? tt_colors[analog_ctx.channel] : 0;
    }

    int button_split = *analog_ctx.value / 37;
    int scale = *analog_ctx.value % 37;
    for (int i = 0; i < 7; i++) {
        uint32_t color = colors[analog_ctx.channel];
        if (i == button_split) {
            color = scale_color(color, scale, 37);
        } else if (i > button_split) {
            color = 0;
        }
        setup_led_button[LED_KEY_1 + i] = color;
    }
}

static struct {
    uint8_t phase; /* 0:H, 1:S, 2:V */
    hsv_t hsv;
    uint8_t *value;
    int16_t start_angle;
    uint16_t keys;
    hsv_t *leds;
} key_ctx;

static void key_apply()
{
    for (int i = 0; i < 11; i++) {
        if (key_ctx.keys & (1 << i)) {
            key_ctx.leds[i] = key_ctx.hsv;
        }
    }
}

static void key_change()
{
    if (JUST_PRESSED(AUX_NO)) {
        quit_mode(false);
        return;
    }

    if (JUST_PRESSED(AUX_YES)) {
        key_ctx.phase++;
        if (key_ctx.phase == 3) {
            key_apply();
            quit_mode(true);
            return;
        }

        if (key_ctx.phase == 1) {
            key_ctx.value = &key_ctx.hsv.s;
        } else {
            key_ctx.value = &key_ctx.hsv.v;
        }
        return;
    }

    key_ctx.keys ^= input.just_pressed;
}

static void key_rotate()
{
    int16_t new_value = *key_ctx.value;
    new_value += input.rotate;
    if (key_ctx.phase > 0) {
        if (new_value < 0) {
            new_value = 0;
        } else if (new_value > 255) {
            new_value = 255;
        }
    }
    *key_ctx.value = (uint8_t)new_value;
}

static void key_loop()
{
    for (int i = 0; i < 11; i ++) {
        if (key_ctx.keys == 0) {
            setup_led_button[i] = button_hsv(key_ctx.hsv) & blink_slow;
        } else if (key_ctx.keys & (1 << i)) {
            setup_led_button[i] = button_hsv(key_ctx.hsv);
        } else {
            setup_led_button[i] = 0;
        }
    }

    uint16_t pos = *key_ctx.value * iidx_cfg->tt_led.num / 256;
    for (unsigned i = 0; i < iidx_cfg->tt_led.num; i++) {
        setup_led_tt[i] = (i == pos) ? tt_rgb32(90, 90, 90, false) : 0;
    }
}

static void key_enter()
{
    key_ctx = (typeof(key_ctx)) {
        .phase = 0,
        .hsv = { .h = 200, .s = 255, .v = 128 },
        .value = &key_ctx.hsv.h,
        .start_angle = input.angle,
        .keys = 0,
        .leds = iidx_cfg->key_on,
    };

    if (current_mode == MODE_KEY_OFF) {
        key_ctx.hsv = (hsv_t) { .h = 60, .s = 255, .v = 5 };
        key_ctx.leds = iidx_cfg->key_off;
    }
}

#define K0_WHITE {.v = 5}
#define K0_SKY {.h = 215, .s = 255, .v = 20}
#define K0_RED {.h = 87, .s = 255, .v = 20}
#define K0_GREEN {.h = 0, .s = 255, .v = 20}

#define K1_WHITE {.v = 200}
#define K1_SKY {.h = 215, .s = 255, .v = 230}
#define K1_RED {.h = 87, .s = 255, .v = 230}
#define K1_GREEN {.h = 0, .s = 255, .v = 230}

#define K0_RAINBOW(x) {.h = 23 * x, .s = 255, .v = 20}
#define K1_RAINBOW(x) {.h = 23 * x, .s = 255, .v = 230}

static struct {
    hsv_t key_off[11];
    hsv_t key_on[11];
} themes[7] = {
    {{ { 0 }, },
     { K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, 
       K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE },
    },
    {{ { 0 }, },
     { K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, 
       K1_RED, K1_GREEN, K1_GREEN, K1_SKY },
    },
    {{ { 0 }, },
     { K1_RED, K1_SKY, K1_RED, K1_SKY, K1_RED, K1_SKY, K1_RED, 
       K1_RED, K1_GREEN, K1_GREEN, K1_GREEN },
    },
    {{ K0_WHITE, K0_WHITE, K0_WHITE, K0_WHITE, K0_WHITE, K0_WHITE, K0_WHITE, 
       K0_WHITE, K0_WHITE, K0_WHITE, K0_WHITE },
     { K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, 
       K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE },
    },
    {{ K0_RED, K0_SKY, K0_RED, K0_SKY, K0_RED, K0_SKY, K0_RED, 
       K0_RED, K0_GREEN, K0_GREEN, K0_GREEN },
     { K1_RED, K1_SKY, K1_RED, K1_SKY, K1_RED, K1_SKY, K1_RED, 
       K1_RED, K1_GREEN, K1_GREEN, K1_GREEN },
    },
    {{ K0_RAINBOW(0), K0_RAINBOW(1), K0_RAINBOW(2), K0_RAINBOW(3),
       K0_RAINBOW(4), K0_RAINBOW(5), K0_RAINBOW(6),
       K0_RAINBOW(7), K0_RAINBOW(8), K0_RAINBOW(9), K0_RAINBOW(10)
     },
     { K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE, 
       K1_WHITE, K1_WHITE, K1_WHITE, K1_WHITE },
    },
    {{ K0_RAINBOW(0), K0_RAINBOW(1), K0_RAINBOW(2), K0_RAINBOW(3),
       K0_RAINBOW(4), K0_RAINBOW(5), K0_RAINBOW(6),
       K0_RAINBOW(7), K0_RAINBOW(8), K0_RAINBOW(9), K0_RAINBOW(10)
     },
     { K1_RAINBOW(0), K1_RAINBOW(1), K1_RAINBOW(2), K1_RAINBOW(3),
       K1_RAINBOW(4), K1_RAINBOW(5), K1_RAINBOW(6),
       K1_RAINBOW(7), K1_RAINBOW(8), K1_RAINBOW(9), K1_RAINBOW(10)
     }
    },
};

static void key_theme_key_change()
{
    for (int i = 0; i < 7; i++) {
        if (JUST_PRESSED(KEY_1 << i)) {
            memcpy(iidx_cfg->key_off, themes[i].key_off, sizeof(iidx_cfg->key_off));
            memcpy(iidx_cfg->key_on, themes[i].key_on, sizeof(iidx_cfg->key_on));
            break;
        }
    }
    check_exit();
}

static void key_theme_loop()
{
    for (int i = 0; i < 11; i++) {
        if (blink_slow) {
            setup_led_button[i] = button_hsv(iidx_cfg->key_on[i]);
         } else {
            setup_led_button[i] = button_hsv(iidx_cfg->key_off[i]);
         }
    }
}

static void tt_theme_key_change()
{
    for (int i = 0; i < 7; i++) {
        if (JUST_PRESSED(KEY_1 << i)) {
            iidx_cfg->tt_led.effect = i;
            break;
        }
    }
    check_exit();
}

static void tt_theme_loop()
{
    for (int i = 0; i < 7; i++) {
        setup_led_button[i] = iidx_cfg->tt_led.effect == i ? SILVER : 0;
    }
}

static struct {
    mode_func key_change;
    mode_func rotate;
    mode_func loop;
    mode_func enter;
} mode_defs[] = {
    [MODE_NONE] = { nop, none_rotate, none_loop, nop},
    [MODE_TURNTABLE] = { tt_key_change, tt_rotate, tt_loop, tt_enter},
    [MODE_ANALOG] = { analog_key_change, analog_rotate, analog_loop, analog_enter},
    [MODE_LEVEL] = { level_key_change, level_rotate, level_loop, nop},
    [MODE_TT_THEME] = { tt_theme_key_change, nop, tt_theme_loop, nop},
    [MODE_KEY_THEME] = { key_theme_key_change, nop, key_theme_loop, nop},
    [MODE_KEY_OFF] = { key_change, key_rotate, key_loop, key_enter},
    [MODE_KEY_ON] = { key_change, key_rotate, key_loop, key_enter},
};

static void join_mode(setup_mode_t new_mode)
{
    cfg_save = *iidx_cfg;
    memset(&setup_led_tt, 0, sizeof(setup_led_tt));
    memset(&setup_led_button, 0, sizeof(setup_led_button));
    current_mode = new_mode;
    mode_defs[current_mode].enter();
    printf("Entering setup %d\n", new_mode);
}

static void quit_mode(bool apply)
{
    if (apply) {
        config_changed();
    } else {
        *iidx_cfg = cfg_save;
    }
    current_mode = MODE_NONE;
    printf("Quit setup %s\n", apply ? "saved." : "discarded.");
}
 
bool setup_run(uint16_t keys, uint16_t angle)
{
    setup_tick_ms = time_us_64() / 1000;
    input.keys = keys;
    input.angle = angle;
    input.just_pressed = keys & ~input.last_keys;
    input.just_released = ~keys & input.last_keys;
    input.rotate = input_delta(input.last_angle);
    if (input.rotate != 0) {
        printf("@ %3d %2x\n", input.rotate, input.angle);
        mode_defs[current_mode].rotate();
    }
    if (input.just_pressed) {
        printf("+ %04x\n", input.just_pressed);
    }
    if (input.just_released) {
        printf("- %04x\n", input.just_released);
    }

    if (input.just_pressed || input.just_released) {
        mode_defs[current_mode].key_change();
    }

    RUN_EVERY_N_MS(blink_fast = ~blink_fast, 100);
    RUN_EVERY_N_MS(blink_slow = ~blink_slow, 500);

    mode_defs[current_mode].loop();

    input.last_keys = keys;
    input.last_angle = angle;

    return current_mode != MODE_NONE;    
}

void setup_init()
{
}