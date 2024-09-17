/*
 * Chu Pico Silder Keys
 * WHowe <github.com/whowechina>
 * 
 * MPR121 CapSense based Keys
 */

#include "touch.h"

#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#include "bsp/board.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "board_defs.h"

#include "config.h"
#include "mpr121.h"

static uint16_t touch[3];
static unsigned touch_counts[36];

static uint8_t touch_map[] = TOUCH_MAP;

void touch_init()
{
    i2c_init(I2C_PORT, I2C_FREQ);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    
    for (int m = 0; m < 3; m++) {
        mpr121_init(MPR121_BASE_ADDR + m);
    }
    touch_update_config();
    memcpy(touch_map, mai_cfg->alt.touch, sizeof(touch_map));
}

const char *touch_key_name(unsigned key)
{
    static char name[3] = { 0 };
    if (key < 18) {
        name[0] = "ABC"[key / 8];
        name[1] = '1' + key % 8;
    } else if (key < 34) {
        name[0] = "DE"[(key - 18) / 8];
        name[1] = '1' + (key - 18) % 8;
    } else {
        return "XX";
    }
    return name;
}

int touch_key_by_name(const char *name)
{
    if (strlen(name) != 2) {
        return -1;
    }
    if (strcasecmp(name, "XX") == 0) {
        return 255;
    }

    int zone = toupper(name[0]) - 'A';
    int id = name[1] - '1';
    if ((zone < 0) || (zone > 4) || (id < 0) || (id > 7)) {
        return -1;
    }
    if ((zone == 2) && (id > 1)) {
        return -1; // C1 and C2 only
    }
    const int offsets[] = { 0, 8, 16, 18, 26 };
    return offsets[zone] + id;
}

int touch_key_channel(unsigned key)
{
    for (int i = 0; i < 36; i++) {
        if (touch_map[i] == key) {
            return i;
        }
    }
    return -1;
}

unsigned touch_key_from_channel(unsigned channel)
{
    if (channel < 36) {
        return touch_map[channel];
    }
    return 0xff;
}

void touch_set_map(unsigned sensor, unsigned key)
{
    if (sensor < 36) {
        touch_map[sensor] = key;
        memcpy(mai_cfg->alt.touch, touch_map, sizeof(mai_cfg->alt.touch));
        config_changed();
    }
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
    touch[0] = mpr121_touched(MPR121_BASE_ADDR) & 0x0fff;
    touch[1] = mpr121_touched(MPR121_BASE_ADDR + 1) & 0x0fff;
    touch[2] = mpr121_touched(MPR121_BASE_ADDR + 2) & 0x0fff;

    remap_reading();

    touch_stat();
}

static bool sensor_ok[3];
bool touch_sensor_ok(unsigned i)
{
    if (i < 3) {
        return sensor_ok[i];
    }
    return false;
}

const uint16_t *touch_raw()
{
    static uint16_t readout[36] = {0};
    // Do not use readout as buffer directly, update readout with buffer when operation finishes
    uint16_t buf[36] = {0};

    for (int i = 0; i < 3; i++) {
        sensor_ok[i] = mpr121_raw(MPR121_BASE_ADDR + i, buf + i * 12, 12);
    }
    memcpy(readout, buf, sizeof(readout));

    return readout;
}

const uint16_t *map_raw_to_zones(const uint16_t* raw)
{
    static uint16_t zones[36];

    for (int i = 0; i < 34; i++) {
        zones[touch_map[i]] = raw[i];
    }
    
    return zones;
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
        mpr121_debounce(MPR121_BASE_ADDR + m,
                        mai_cfg->sense.debounce_touch,
                        mai_cfg->sense.debounce_release);
        mpr121_sense(MPR121_BASE_ADDR + m,
                     mai_cfg->sense.global,
                     mai_cfg->sense.zones + m * 12,
                     m != 2 ? 12 : 10);
        mpr121_filter(MPR121_BASE_ADDR + m,
                      mai_cfg->sense.filter >> 6,
                      (mai_cfg->sense.filter >> 4) & 0x03,
                      mai_cfg->sense.filter & 0x07);
    }
}
