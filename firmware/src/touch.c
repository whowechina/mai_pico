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

static uint16_t touch[3];
static unsigned touch_counts[36];

enum touch_pads {
    A1 = 0, A2, A3, A4, A5, A6, A7, A8,
    B1, B2, B3, B4, B5, B6, B7, B8,
    C1, C2, D1, D2, D3, D4, D5, D6, D7, D8,
    E1, E2, E3, E4, E5, E6, E7, E8,
};

static unsigned touch_map[] = TOUCH_MAP;

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

static uint64_t touch_reading;

static void remap_reading()
{
    uint64_t map = 0;
    for (int m = 0; m < 3; m++) {
        for (int i = 0; i < 12; i++) {
            if (touch[m] & (1 << i)) {
                map |= 1ULL << touch_map[m * 12 + i];
            }
        }
    }
    touch_reading = map;
}

static void touch_stat()
{
    static uint64_t last_reading;

    uint64_t just_touched = touch_reading & ~last_reading;
    last_reading = touch_reading;

    for (int i = 0; i < 34; i++) {
        if (just_touched & (1ULL << i)) {
            touch_counts[i]++;
        }
    }
}

void touch_update()
{
    touch[0] = mpr121_touched(MPR121_ADDR) & 0x0fff;
    touch[1] = mpr121_touched(MPR121_ADDR + 1) & 0x0fff;
    touch[2] = mpr121_touched(MPR121_ADDR + 2) & 0x0fff;

    remap_reading();

    touch_stat();
}

const uint16_t *touch_raw()
{
    static uint16_t readout[36];
    uint16_t buf[36];
    mpr121_raw(MPR121_ADDR, buf, 12);
    mpr121_raw(MPR121_ADDR + 1, buf + 12, 12);
    mpr121_raw(MPR121_ADDR + 2, buf + 24, 10);

    for (int i = 0; i < 34; i++) {
        readout[touch_map[i]] = buf[i];
    }
    return readout;
}

bool touch_touched(unsigned key)
{
    if (key >= 34) {
        return 0;
    }
    return touch_reading & (1ULL << key);
}

uint64_t touch_touchmap()
{
    return touch_reading;
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
                                      mai_cfg->sense.zones + m * 12,
                                      m != 2 ? 12 : 10);
        mpr121_filter(MPR121_ADDR + m, mai_cfg->sense.filter >> 6,
                                       (mai_cfg->sense.filter >> 4) & 0x03,
                                       mai_cfg->sense.filter & 0x07);
    }
}
