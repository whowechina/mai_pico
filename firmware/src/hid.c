#include <stdint.h>
#include <stdbool.h>

#include "board_defs.h"

#include "tusb.h"
#include "usb_descriptors.h"
#include "button.h"
#include "config.h"
#include "hid.h"

struct __attribute__((packed)) {
    uint16_t buttons;
} hid_joy;

struct __attribute__((packed)) {
    uint8_t modifier;
    uint8_t keymap[15];
} hid_nkro;

static void report_usb_hid()
{
    if (tud_hid_ready()) {
        if (mai_cfg->hid.joy) {
            hid_joy.buttons = button_read();
            tud_hid_n_report(0, REPORT_ID_JOYSTICK, &hid_joy, sizeof(hid_joy));
        }
        if (mai_cfg->hid.nkro) {
            tud_hid_n_report(1, 0, &hid_nkro, sizeof(hid_nkro));
        }
    }
}

const char keymap_p1[10] = BUTTON_NKRO_MAP_P1;
const char keymap_p2[10] = BUTTON_NKRO_MAP_P2;

static void gen_nkro_report()
{
    if (!mai_cfg->hid.nkro) {
        return;
    }

    uint16_t buttons = button_read();
    const char *keymap = (mai_cfg->hid.nkro == 2) ? keymap_p2 : keymap_p1;
    for (int i = 0; i < 8; i++) {
        uint8_t code = keymap[i];
        uint8_t byte = code / 8;
        uint8_t bit = code % 8;
        if (buttons & (1 << i)) {
            hid_nkro.keymap[byte] |= (1 << bit);
        } else {
            hid_nkro.keymap[byte] &= ~(1 << bit);
        }
    }
}

void hid_update()
{
    gen_nkro_report();
    report_usb_hid();
}