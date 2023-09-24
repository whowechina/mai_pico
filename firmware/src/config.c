/*
 * Controller Config and Runtime Data
 * WHowe <github.com/whowechina>
 * 
 * Config is a global data structure that stores all the configuration
 * Runtime is something to share between files.
 */

#include "config.h"
#include "save.h"

mai_cfg_t *mai_cfg;

static mai_cfg_t default_cfg = {
    .colors = {
        .key_on = 0x00FF00,
        .key_off = 0x000000,
    },
    .style = {
        .key = 0,
        .level = 7,
    },
    .sense = {
        .filter = 0x11,
        .debounce_touch = 1,
        .debounce_release = 2,
     },
    .hid = {
        .joy = 1,
        .nkro = 0,
    },
};

mai_runtime_t *mai_runtime;

static void config_loaded()
{
    if (mai_cfg->style.level > 10) {
        mai_cfg->style.level = default_cfg.style.level;
        config_changed();
    }
    if ((mai_cfg->sense.filter & 0x0f) > 3 ||
        ((mai_cfg->sense.filter >> 4) & 0x0f) > 3) {
        mai_cfg->sense.filter = default_cfg.sense.filter;
        config_changed();
    }
    if ((mai_cfg->sense.global > 9) || (mai_cfg->sense.global < -9)) {
        mai_cfg->sense.global = default_cfg.sense.global;
        config_changed();
    }
    for (int i = 0; i < 32; i++) {
        if ((mai_cfg->sense.keys[i] > 9) || (mai_cfg->sense.keys[i] < -9)) {
            mai_cfg->sense.keys[i] = default_cfg.sense.keys[i];
            config_changed();
        }
    }
    if ((mai_cfg->sense.debounce_touch > 7) |
        (mai_cfg->sense.debounce_release > 7)) {
        mai_cfg->sense.debounce_touch = default_cfg.sense.debounce_touch;
        mai_cfg->sense.debounce_release = default_cfg.sense.debounce_release;
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
