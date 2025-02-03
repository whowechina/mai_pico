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

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"

#include "tusb.h"
#include "usb_descriptors.h"

#include "aime.h"
#include "nfc.h"

#include "board_defs.h"

#include "touch.h"
#include "button.h"
#include "rgb.h"

#include "save.h"
#include "config.h"
#include "cli.h"
#include "commands.h"
#include "io.h"
#include "hid.h"

static void button_lights_clear()
{
    for (int i = 0; i < 8; i++) {
        rgb_set_button(i, 0, 0);
    }
}

static void button_lights_rainbow()
{
    static uint16_t loop = 0;
    loop++;
    uint16_t buttons = button_read();
    for (int i = 0; i < 8; i++) {
        uint8_t phase = (i * 256 + loop) / 8;
        uint32_t color;
        if (buttons & (1 << i)) {
            color = rgb32_from_hsv(phase, 64, 255);
        } else {
            color = rgb32_from_hsv(phase, 240, 20);
        }
        rgb_set_button(i, color, 0);
    }
}

static void run_lights()
{
    static bool was_rainbow = true;
    bool go_rainbow = !io_is_active() && !aime_is_active();

    if (go_rainbow) {
        button_lights_rainbow();
    } else if (was_rainbow) {
        button_lights_clear();
    }

    was_rainbow = go_rainbow;

    rgb_set_aime(aime_led_color());
}

const int aime_intf = 3;
static void cdc_aime_putc(uint8_t byte)
{
    tud_cdc_n_write(aime_intf, &byte, 1);
    tud_cdc_n_write_flush(aime_intf);
}

static void aime_run()
{
    if (tud_cdc_n_available(aime_intf)) {
        uint8_t buf[32];
        uint32_t count = tud_cdc_n_read(aime_intf, buf, sizeof(buf));

        for (int i = 0; i < count; i++) {
            aime_feed(buf[i]);
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

static void runtime_ctrl()
{
    /* Just use long-press COIN to reset touch in runtime */
    static bool applied = false;
    static uint64_t press_time = 0;
    static bool last_coin_button = false;
    bool coin_button = button_read() & (1 << 11);

    if (coin_button) {
        if (!last_coin_button) {
            press_time = time_us_64();
            applied = false;
        }
        if (!applied && (time_us_64() - press_time > 2000000)) {
            touch_sensor_init();
            applied = true;
        }
    }

    last_coin_button = coin_button;
}

static void core0_loop()
{
    uint64_t next_frame = time_us_64();

    while(1) {
        tud_task();
        io_update();

        cli_run();
        aime_run();
        save_loop();
        cli_fps_count(0);

        sleep_until(next_frame);
        next_frame += 1000;

        touch_update();
        button_update();

        runtime_ctrl();
        hid_update();
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

    save_init(board_id_32() ^ 0xcafe1111, &core1_io_lock);

    if (!mai_cfg->hid.io4) {
        usb_descriptors_disable_io4();
    }

    touch_init();
    button_init();
    rgb_init();

    nfc_attach_i2c(I2C_PORT);
    nfc_init();
    aime_init(cdc_aime_putc);
    aime_sub_mode(mai_cfg->aime.mode);
    aime_virtual_aic(mai_cfg->aime.virtual_aic);

    cli_init("mai_pico>", "\n   << Mai Pico Controller >>\n"
                            " https://github.com/whowechina\n\n");
    commands_init();

    mai_runtime.key_stuck = button_is_stuck();
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
    hid_proc(buffer, bufsize);
}
