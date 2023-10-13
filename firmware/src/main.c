/*
 * Controller Main
 * WHowe <github.com/whowechina>
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "bsp/board.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"

#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"

#include "tusb.h"
#include "usb_descriptors.h"

#include "board_defs.h"

#include "touch.h"
#include "button.h"
#include "rgb.h"

#include "save.h"
#include "config.h"
#include "cli.h"
#include "commands.h"
#include "io.h"

struct __attribute__((packed)) {
    uint16_t buttons;
} hid_joy;

struct __attribute__((packed)) {
    uint8_t modifier;
    uint8_t keymap[15];
} hid_nkro, sent_hid_nkro;

void report_usb_hid()
{
    if (tud_hid_ready()) {
        if (mai_cfg->hid.joy) {
            hid_joy.buttons = button_read();
            tud_hid_n_report(0, REPORT_ID_JOYSTICK, &hid_joy, sizeof(hid_joy));
        }
        if (mai_cfg->hid.nkro) {
            sent_hid_nkro = hid_nkro;
            tud_hid_n_report(1, 0, &sent_hid_nkro, sizeof(sent_hid_nkro));
        }
    }
}

const uint8_t keycode_table[128][2] = { HID_ASCII_TO_KEYCODE };
const char keymap[8] = BUTTON_NKRO_MAP; // 8 buttons
static void gen_nkro_report()
{
    uint16_t buttons = button_read();
    for (int i = 0; i < 8; i++) {
        uint8_t code = keycode_table[keymap[i]][1];
        uint8_t byte = code / 8;
        uint8_t bit = code % 8;
        if (buttons & (1 << i)) {
            hid_nkro.keymap[byte] |= (1 << bit);
        } else {
            hid_nkro.keymap[byte] &= ~(1 << bit);
        }
    }
}

static uint64_t last_hid_time = 0;

static void run_lights()
{
    uint64_t now = time_us_64();
    if (last_hid_time != 0 && now - last_hid_time < 1000000) {
        return;
    }

    if (io_last_io_time() != 0 && now - io_last_io_time() < 60000000) {
        return;
    }

    uint16_t buttons = button_read();
    uint16_t touch = touch_touchmap();
    for (int i = 0; i < 8; i++) {
        uint32_t color = 0;
        if (buttons & (1 << i)) {
            color |= 0x00ff00;
        }
        if (touch & (1 << i)) {
            color |= 0xff0000;
        }
        rgb_set_button_color(i, color);
    }
}

static mutex_t core1_io_lock;
static void core1_loop()
{
    while (1) {
        if (mutex_try_enter(&core1_io_lock, NULL)) {
            run_lights();
            rgb_update();
            mutex_exit(&core1_io_lock);
        }
        cli_fps_count(1);
        sleep_ms(1);
    }
}

static void core0_loop()
{
    static uint64_t next_frame = 0;

    while(1) {
        tud_task();
        io_update();

        cli_run();
        save_loop();
        cli_fps_count(0);

        sleep_until(next_frame);
        next_frame = time_us_64() + 1000; // 1KHz

        touch_update();
        button_update();

        gen_nkro_report();
        report_usb_hid();
    }
}

void init()
{
    sleep_ms(50);
    set_sys_clock_khz(150000, true);
    board_init();
    tusb_init();
    stdio_init_all();

    config_init();
    mutex_init(&core1_io_lock);
    save_init(0xca34cafe, &core1_io_lock);

    touch_init();
    button_init();
    rgb_init();

    cli_init("mai_pico>", "\n   << Mai Pico Controller >>\n"
                            " https://github.com/whowechina\n\n");
    commands_init();
}

int main(void)
{
    init();
    multicore_launch_core1(core1_loop);
    core0_loop();
    return 0;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen)
{
    printf("Get from USB %d-%d\n", report_id, report_type);
    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize)
{
}
