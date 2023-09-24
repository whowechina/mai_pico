/*
 * Controller Config
 * WHowe <github.com/whowechina>
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

typedef struct __attribute__((packed)) {
    struct {
        uint32_t key_on;
        uint32_t key_off;
    } colors;
    struct {
        uint8_t key;
        uint8_t level;
    } style;
    struct {
        int8_t filter;
        int8_t global;
        uint8_t debounce_touch;
        uint8_t debounce_release;
        int8_t keys[34];
    } sense;
    struct {
        uint8_t joy : 4;
        uint8_t nkro : 4;
    } hid;
} mai_cfg_t;

typedef struct {
    uint16_t fps[2];
} mai_runtime_t;

extern mai_cfg_t *mai_cfg;
extern mai_runtime_t *mai_runtime;

void config_init();
void config_changed(); // Notify the config has changed
void config_factory_reset(); // Reset the config to factory default

#endif
