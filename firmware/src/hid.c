#include <stdint.h>
#include <stdbool.h>

#include "board_defs.h"

#include "tusb.h"
#include "usb_descriptors.h"
#include "button.h"
#include "config.h"
#include "hid.h"

struct __attribute__((packed)) {
    uint16_t adcs[8];
    uint16_t spinners[4];
    uint16_t chutes[2];
    uint16_t buttons[2];
    uint8_t system_status;
    uint8_t usb_status;
    uint8_t padding[29];
} hid_io4, sent_hid_io4;

struct __attribute__((packed)) {
    uint8_t modifier;
    uint8_t keymap[15];
} hid_nkro, sent_hid_nkro;

static uint16_t native_to_io4(uint16_t button)
{
    static const int target_pos[] = { 2, 3, 0, 15, 14, 13, 12, 11, 9, 6, 1 };
    uint16_t io4btn = 0;
    for (int i = 0; i < 8; i++) {
        bool pressed = button & (1 << i);
        io4btn |= pressed ? 0 : (1 << target_pos[i]);
    }
    for (int i = 8; i < 11; i++) {
        bool pressed = button & (1 << i);
        io4btn |= pressed ? (1 << target_pos[i]) : 0;
    }
    return io4btn;
}

static void gen_io4_report()
{
    static uint16_t last_buttons = 0;
    uint16_t buttons = button_read();
    hid_io4.buttons[0] = native_to_io4(buttons);
    hid_io4.buttons[1] = native_to_io4(0);
    if ((last_buttons ^ buttons) & (1 << 11)) {
        if (buttons & (1 << 11)) {
            // just pressed coin button
            hid_io4.chutes[0] += 0x100;
        }
    }
    last_buttons = buttons;
}

const char keymap_p1[] = BUTTON_NKRO_MAP_P1;
const char keymap_p2[] = BUTTON_NKRO_MAP_P2;

static void gen_nkro_report()
{
    uint16_t buttons = button_read();
    const char *keymap = (mai_cfg->hid.nkro == 2) ? keymap_p2 : keymap_p1;
    for (int i = 0; i < button_num(); i++) {
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
    if (!tud_hid_ready()) {
        return;
    }

    if (mai_cfg->hid.io4) {
        gen_io4_report();
        static uint64_t next_periodic_report = 0;
        uint64_t now = time_us_64();
        /* Somehow edge-triggering doesn't pass self-test, so periodic report
           is also there, with a lower rate that doesn't crowd the bus. */
        if ((memcmp(&hid_io4, &sent_hid_io4, sizeof(hid_io4)) != 0) ||
            (now > next_periodic_report)) {
            next_periodic_report = now + 4000; // 250Hz periodic report
            if (tud_hid_report(REPORT_ID_JOYSTICK, &hid_io4, sizeof(hid_io4))) {
                sent_hid_io4 = hid_io4;
            }
        }
    }

    if (mai_cfg->hid.nkro && !mai_runtime.key_stuck) {
        gen_nkro_report();
        if ((memcmp(&hid_nkro, &sent_hid_nkro, sizeof(hid_nkro)) != 0) &&
            tud_hid_n_report(1, 0, &hid_nkro, sizeof(hid_nkro))) {
            sent_hid_nkro = hid_nkro;
        }
    }
}

typedef struct __attribute__((packed)) {
    uint8_t report_id;
    uint8_t cmd;
    uint8_t payload[62];
} hid_output_t;

void hid_proc(const uint8_t *data, uint8_t len)
{
    hid_output_t *output = (hid_output_t *)data;
    if (output->report_id == REPORT_ID_OUTPUT) {
        switch (output->cmd) {
            case 0x01: // Set Timeout
            case 0x02: // Set Sampling Count
                hid_io4.system_status = 0x30;
                break;
            case 0x03: // Clear Board Status
                hid_io4.chutes[0] = 0;
                hid_io4.chutes[1] = 0;
                hid_io4.system_status = 0x00;
                break;
            case 0x04: // Set General Output
            case 0x41: // I don't know what this is
                break;
            default:
                printf("USB unknown cmd: %d\n", output->cmd);
                break;
        }
    }
}
