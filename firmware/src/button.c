/*
 * Mai Controller Buttons
 * WHowe <github.com/whowechina>
 * 
 */

#include "button.h"

#include <stdint.h>
#include <stdbool.h>

#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"

#include "config.h"
#include "board_defs.h"

static const uint8_t gpio_def[] = BUTTON_DEF;
static uint8_t gpio_real[] = BUTTON_DEF;

#define BUTTON_NUM (sizeof(gpio_def))

static bool sw_val[BUTTON_NUM]; /* true if pressed */
static uint64_t sw_freeze_time[BUTTON_NUM];

void button_init()
{
    for (int i = 0; i < BUTTON_NUM; i++)
    {
        sw_val[i] = false;
        sw_freeze_time[i] = 0;
        uint8_t gpio = mai_cfg->alt.buttons[i];
        if (gpio > 29) {
            gpio = gpio_def[i];
        }
        gpio_real[i] = gpio;
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

uint8_t button_gpio(int id)
{
    if (id >= BUTTON_NUM) {
        return 0xff;
    }
    return gpio_real[id];
}

static uint16_t button_reading;

/* If a switch flips, it freezes for a while */
#define DEBOUNCE_FREEZE_TIME_US 3000
void button_update()
{
    uint64_t now = time_us_64();
    uint16_t buttons = 0;

    for (int i = BUTTON_NUM - 1; i >= 0; i--) {
        bool sw_pressed = !gpio_get(gpio_real[i]);
        
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

    button_reading = buttons;
}

uint16_t button_read()
{
    return button_reading;
}
