/*
 * Mai Pico Silder Keys
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

static uint16_t baseline[36];
static int16_t error[36];
static uint16_t readout[36];
static bool touched[36];
static uint16_t touch[3];

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
    touch[0] = mpr121_touched(MPR121_ADDR);
    touch[1] = mpr121_touched(MPR121_ADDR + 1);
    touch[2] = mpr121_touched(MPR121_ADDR + 2);
}

bool touch_touched(unsigned key)
{
    if (key >= 32) {
        return 0;
    }
    return touch[key / 12] & (1 << (key % 12));
}

void touch_update_config()
{
    for (int m = 0; m < 3; m++) {
        mpr121_debounce(MPR121_ADDR + m, mai_cfg->sense.debounce_touch,
                                         mai_cfg->sense.debounce_release);
        mpr121_sense(MPR121_ADDR + m, mai_cfg->sense.global, mai_cfg->sense.keys + m * 12);
        mpr121_filter(MPR121_ADDR + m, mai_cfg->sense.filter & 0x0f,
                                       (mai_cfg->sense.filter >> 4) & 0x0f);
    }
}
