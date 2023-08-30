/*
 * Controller Buttons
 * WHowe <github.com/whowechina>
 * 
 * A button consists of a switch and an LED
 */

#include "buttons.h"

#include <stdint.h>
#include <stdbool.h>

#include "bsp/board.h"
#include "hardware/gpio.h"

#include "board_defs.h"

static const uint8_t BUTTON_GPIOS[] = BUTTON_DEF;
#define BUTTON_NUM (sizeof(BUTTON_GPIOS))

static bool sw_val[BUTTON_NUM]; /* true if pressed */
static uint64_t sw_freeze_time[BUTTON_NUM];

#define LIMIT_MAX(a, max, def) { if (a > max) a = def; }

void button_init()
{
    for (int i = 0; i < BUTTON_NUM; i++) {
        sw_val[i] = false;
        sw_freeze_time[i] = 0;
        int8_t gpio = BUTTON_GPIOS[i];
        gpio_init(gpio);
        gpio_set_function(gpio, GPIO_FUNC_SIO);
        gpio_set_dir(gpio, GPIO_IN);
        gpio_pull_up(gpio);
    }
}

uint8_t button_num()
{
    return BUTTON_NUM;
}

uint8_t button_gpio(uint8_t id)
{
    return BUTTON_GPIOS[id];
}

/* If a switch flips, it freezes for a while */
#define DEBOUNCE_FREEZE_TIME_US 5000
uint16_t button_read()
{
    uint64_t now = time_us_64();
    uint16_t buttons = 0;

    for (int i = BUTTON_NUM - 1; i >= 0; i--) {
        bool sw_pressed = !gpio_get(BUTTON_GPIOS[i]);
        
        if (now >= sw_freeze_time[i]) {
            if (sw_pressed != sw_val[i]) {
                sw_val[i] = sw_pressed;
                sw_freeze_time[i] = now + DEBOUNCE_FREEZE_TIME_US;
            }
        }

        buttons <<= 1;
        if (sw_val[i]) {
            buttons |= 1;
        }
    }

    return buttons;
}
