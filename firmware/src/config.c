/*
 * Controller Config and Runtime Data
 * WHowe <github.com/whowechina>
 * 
 * Config is a global data structure that stores all the configuration
 * Runtime is something to share between files.
 */

#include <string.h>

#include "config.h"
#include "save.h"
#include "touch.h"

mai_cfg_t *mai_cfg;

static mai_cfg_t default_cfg = {
    .color = {
        .key_on = 0xc0c0c0,
        .key_off = 0x080808,
        .level = 127,
    },
    .sense = {
        .filter = 0x10,
        .debounce_touch = 1,
        .debounce_release = 2,
     },
    .hid = {
        .joy = 0,
        .nkro = 1,
    },
    .rgb = {
        .per_button = 1,
        .per_aux = 1,
    },
    .alt = {
        .buttons = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
        .touch = TOUCH_MAP,
    },
    .aime = {
        .mode = 0,
        .virtual_aic = 0,
    }
};

mai_runtime_t mai_runtime;

static inline bool in_range(int val, int min, int max)
{
    return (val >= min) && (val <= max);
}

static bool touch_map_valid()
{
    uint64_t mask = 0;
    for (int i = 0; i < sizeof(mai_cfg->alt.touch); i++) {
        if (mai_cfg->alt.touch[i] < 34) {
            mask |= 1ULL << mai_cfg->alt.touch[i];
        }
    }
    int keys = 0;
    for (int i = 0; i < 34; i++) {
        if (mask & (1ULL << i)) {
            keys++;
        }
    }
    return keys > 10; // bad data results in low touch key coverage
}

static void config_loaded()
{
    if ((mai_cfg->sense.filter & 0x0f) > 3 ||
        ((mai_cfg->sense.filter >> 4) & 0x0f) > 3) {
        mai_cfg->sense = default_cfg.sense;
        config_changed();
    }
    if (!in_range(mai_cfg->sense.global, -9, 9)) {
        mai_cfg->sense = default_cfg.sense;
        config_changed();
    }
    for (int i = 0; i < 34; i++) {
        if (!in_range(mai_cfg->sense.zones[i], -9, 9)) {
            mai_cfg->sense = default_cfg.sense;
            config_changed();
            break;
        }
    }
    if (!in_range(mai_cfg->sense.debounce_touch, 0, 7) ||
        !in_range(mai_cfg->sense.debounce_release, 0, 7)) {
        mai_cfg->sense = default_cfg.sense;
        config_changed();
    }

    if (!in_range(mai_cfg->rgb.per_button, 1, 16) ||
        !in_range(mai_cfg->rgb.per_aux, 1, 16)) {
        mai_cfg->rgb = default_cfg.rgb;
        config_changed();
    }

    if (!touch_map_valid()) {
        memcpy(mai_cfg->alt.touch, default_cfg.alt.touch,
               sizeof(mai_cfg->alt.touch));
        config_changed();
    }
}

void config_changed()
{
    save_request(false);
}

void config_factory_reset()
{
    *mai_cfg = default_cfg;
    save_request(true);
}

void config_init()
{
    mai_cfg = (mai_cfg_t *)save_alloc(sizeof(*mai_cfg), &default_cfg, config_loaded);
}
