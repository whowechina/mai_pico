/*
 * Chu Pico Silder Keys
 * WHowe <github.com/whowechina>
 * 
 * MPR121 CapSense based Keys
 */

#include "touch.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "bsp/board.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "board_defs.h"

#include "config.h"
#include "mpr121.h"

#define MPR121_ADDR 0x5A

static uint16_t readout[36];
static uint16_t touch[3];
static unsigned touch_counts[36];

void touch_init()
{
    i2c_init(I2C_PORT, I2C_FREQ);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    
    for (int m = 0; m < 3; m++) {
        mpr121_init(MPR121_ADDR + m);
    }
    touch_update_config();
}

void touch_update()
{
    static uint16_t last_touched[3];

    touch[0] = mpr121_touched(MPR121_ADDR);
    touch[1] = mpr121_touched(MPR121_ADDR + 1);
    touch[2] = mpr121_touched(MPR121_ADDR + 2);

    for (int m = 0; m < 3; m++) {
        uint16_t just_touched = touch[m] & ~last_touched[m];
        last_touched[m] = touch[m];
        for (int i = 0; i < 12; i++) {
            if (just_touched & (1 << i)) {
                touch_counts[m * 12 + i]++;
            }
        }
    }
}

const uint16_t *touch_raw()
{
    mpr121_raw(MPR121_ADDR, readout, 12);
    mpr121_raw(MPR121_ADDR + 1, readout + 12, 12);
    mpr121_raw(MPR121_ADDR + 2, readout + 24, 12);
    return readout;
}

bool touch_touched(unsigned key)
{
    if (key >= 34) {
        return 0;
    }
    return touch[key / 12] & (1 << (key % 12));
}

unsigned touch_count(unsigned key)
{
    if (key >= 34) {
        return 0;
    }
    return touch_counts[key];
}

void touch_reset_stat()
{
    memset(touch_counts, 0, sizeof(touch_counts));
}

void touch_update_config()
{
    for (int m = 0; m < 3; m++) {
        mpr121_debounce(MPR121_ADDR + m, mai_cfg->sense.debounce_touch,
                                         mai_cfg->sense.debounce_release);
        mpr121_sense(MPR121_ADDR + m, mai_cfg->sense.global,
                                      mai_cfg->sense.keys + m * 12);
        mpr121_filter(MPR121_ADDR + m, mai_cfg->sense.filter >> 6,
                                       (mai_cfg->sense.filter >> 4) & 0x03,
                                       mai_cfg->sense.filter & 0x07);
    }
}
