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
#include "save.h"
#include "config.h"
#include "cli.h"
#include "commands.h"

#include "rgb.h"

struct __attribute__((packed)) {
    uint16_t buttons; // 16 buttons; see JoystickButtons_t for bit mapping
    uint8_t  HAT;    // HAT switch; one nibble w/ unused nibble
    uint32_t axis;  // slider touch data
    uint8_t  VendorSpec;
} hid_joy;

struct __attribute__((packed)) {
    uint8_t modifier;
    uint8_t keymap[15];
} hid_nkro, sent_hid_nkro;

void report_usb_hid()
{
    if (tud_hid_ready()) {
        hid_joy.HAT = 0;
        hid_joy.VendorSpec = 0;
        if (mai_cfg->hid.joy) {
            tud_hid_n_report(0x00, REPORT_ID_JOYSTICK, &hid_joy, sizeof(hid_joy));
        }
        if (mai_cfg->hid.nkro &&
            (memcmp(&hid_nkro, &sent_hid_nkro, sizeof(hid_nkro)) != 0)) {
            sent_hid_nkro = hid_nkro;
            tud_hid_n_report(0x02, 0, &sent_hid_nkro, sizeof(sent_hid_nkro));
        }
    }
}

static void gen_joy_report()
{
    hid_joy.axis = 0;
    for (int i = 0; i < 16; i++) {
        if (touch_touched(i * 2)) {
            hid_joy.axis |= 1 << (30 - i * 2);
        }
        if (touch_touched(i * 2 + 1)) {
            hid_joy.axis |= 1 << (31 - i * 2);
        }

    }
    hid_joy.axis ^= 0x80808080; // some magic number from CrazyRedMachine
}

const uint8_t keycode_table[128][2] = { HID_ASCII_TO_KEYCODE };
const char keymap[38 + 1] = NKRO_KEYMAP; // 32 keys, 6 air keys, 1 terminator
static void gen_nkro_report()
{
    for (int i = 0; i < 32; i++) {
        uint8_t code = keycode_table[keymap[i]][1];
        uint8_t byte = code / 8;
        uint8_t bit = code % 8;
        if (touch_touched(i)) {
            hid_nkro.keymap[byte] |= (1 << bit);
        } else {
            hid_nkro.keymap[byte] &= ~(1 << bit);
        }
    }
    for (int i = 0; i < 6; i++) {
        uint8_t code = keycode_table[keymap[32 + i]][1];
        uint8_t byte = code / 8;
        uint8_t bit = code % 8;
        if (hid_joy.buttons & (1 << i)) {
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
    if (now - last_hid_time < 1000000) {
        return;
    }

    uint32_t colors[5] = { 0xff0000, 0x00ff00, 0x0000ff, 0xffff00, 0xffffff };

    for (int i = 0; i < 8; i++) {
        rgb_set_color(i, 0);
    }
    for (int i = 0; i < 34; i++) {
        if (touch_touched(i)) {
            if (i < 32) {
                rgb_set_color((i + 16) / 4 % 8, colors[i % 4]);
            } else {
                rgb_set_color(i - 32, colors[4]);
            }
        }
    }
//    for (int i = 0; i < 15; i++) {
//        uint32_t color = rgb32_from_hsv(i * 255 / 8, 255, 16);
//        rgb_set_color(i, color);
//    }

    for (int i = 0; i < 34; i++) {
//        bool r = touch_touched(i * 2);
//        bool g = touch_touched(i * 2 + 1);
//        rgb_set_color(30 - i * 2, rgb32(r ? 80 : 0, g ? 80 : 0, 0, false));
    }
}


static void echo_serial_port(uint8_t itf, uint8_t buf[], uint32_t count)
{
    //tud_cdc_n_write_char(itf, buf[i]);
    //tud_cdc_n_write_flush(itf);
}

static void cdc_task(void)
{
    uint8_t itf;

    for (itf = 1; itf < CFG_TUD_CDC; itf++)
    {
        // connected() check for DTR bit
        // Most but not all terminal client set this when making connection
        // if ( tud_cdc_n_connected(itf) )
        if ( tud_cdc_n_available(itf) )
        {
            uint8_t buf[64];

            uint32_t count = tud_cdc_n_read(itf, buf, sizeof(buf));

            if (itf == 1) {
                printf("1 TXT:", itf);
                for (int i = 0; i < count; i++) {
                    printf("%c", buf[i]);
                }
                printf("\n");
            } else if (itf == 2) {
                printf("2 HEX:", itf);
                for (int i = 0; i < count; i++) {
                    printf(" %02x", buf[i]);
                }
                printf("\n");
            }
        }
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
    while(1) {
        tud_task();
        cdc_task();

        cli_run();
        save_loop();
        cli_fps_count(0);

        touch_update();

        gen_joy_report();
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
    if (report_type == HID_REPORT_TYPE_OUTPUT) {
        last_hid_time = time_us_64();
        return;
    } 
    
    if (report_type == HID_REPORT_TYPE_FEATURE) {
        if (report_id == REPORT_ID_LED_COMPRESSED) {
        }
        last_hid_time = time_us_64();
        return;
    }
}
