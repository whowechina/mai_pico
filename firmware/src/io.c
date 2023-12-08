#include <stdint.h>
#include <stdbool.h>

#include "io.h"
#include "tusb.h"
#include "usb_descriptors.h"

#include "touch.h"
#include "rgb.h"

static struct {
    bool stat;
    uint64_t last_io_time;
    int touch_interface;
} ctx = { false, 0, 0 };

typedef union {
    uint8_t raw[28];
    struct {
        uint8_t body;
        uint8_t ext;
        uint8_t side;
    };
    struct {
        uint8_t index;
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };
    struct {
        uint8_t start;
        uint8_t len;
        uint8_t skip;
        uint8_t mr;
        uint8_t mg;
        uint8_t mb;
        uint8_t speed;                    
    };
    uint8_t rgb[11][3];
} led_data_t;

typedef struct {
    int interface;
    bool connected;
    bool in_cmd;
    bool is_touch;
    bool escape;
    union {
        uint8_t buf[48];
        struct {
            struct {
                uint8_t dst;
                uint8_t src;
                uint8_t len;
                uint8_t cmd;
            } hdr;
            led_data_t data;
        };
    };
    uint8_t len;
    uint8_t checksum;
} cdc_t;

static cdc_t cdc[2] = {
    { .interface = 1 },
    { .interface = 2 },
};

static void touch_cmd(cdc_t *cdc)
{
    cdc->in_cmd = false;
    if (cdc->len != 4) {
        return;
    }
    cdc->len = 0;
    ctx.last_io_time = time_us_64();

    ctx.touch_interface = cdc->interface;

    printf("Touch CMD: %*s -> ", 4, cdc->buf);
    switch (cdc->buf[2]) {
        case 'E':
            printf("RSET\n");
            break;
        case 'L':
            printf("HALT\n");
            ctx.stat = false;
            break;
        case 'A':
            printf("STAT\n");
            ctx.stat = true;
            break;
        case 'r':
            printf("Ratio\n");
            tud_cdc_n_write_char(cdc->interface, '(');
            tud_cdc_n_write_char(cdc->interface, cdc->buf[0]); //L,R
            tud_cdc_n_write_char(cdc->interface, cdc->buf[1]); //sensor
            tud_cdc_n_write_char(cdc->interface, 'r');
            tud_cdc_n_write_char(cdc->interface, cdc->buf[3]); // Ratio
            tud_cdc_n_write_char(cdc->interface, ')');
            tud_cdc_n_write_flush(cdc->interface);
            break;
        case 'k':
            printf("Sense\n");
            tud_cdc_n_write_char(cdc->interface, '(');
            tud_cdc_n_write_char(cdc->interface, cdc->buf[0]); //L,R
            tud_cdc_n_write_char(cdc->interface, cdc->buf[1]); //sensor
            tud_cdc_n_write_char(cdc->interface, 'k');
            tud_cdc_n_write_char(cdc->interface, cdc->buf[3]); // Ratio
            tud_cdc_n_write_char(cdc->interface, ')');
            tud_cdc_n_write_flush(cdc->interface);
            break;
        default:
            printf("Unknown\n");
            return;
    }
}

static void led_cmd(cdc_t *cdc)
{
    cdc->in_cmd = false;
    cdc->len = 0;
    ctx.last_io_time = time_us_64();

    switch (cdc->hdr.cmd) {
        case 0x31:
            printf("8b\n");
            uint32_t color = rgb32(cdc->data.r, cdc->data.g, cdc->data.b, false);
            rgb_set_button_color(cdc->data.index, color);          
            break;
        case 0x32:
            printf("8bM\n");
            for (int i = 0; i < cdc->data.len; i++) {
                rgb_set_button_color(i + cdc->data.start, rgb32(cdc->data.mr, cdc->data.mg, cdc->data.mb, false));
            }
            break;
        case 0x33:
            printf("8bMF\n");
            for (int i = 0; i < cdc->data.len; i++) {
                rgb_set_button_color(i + cdc->data.start, rgb32(cdc->data.mr, cdc->data.mg, cdc->data.mb, false));
            }
            break;
        case 0x39:
            printf("Fet\n");
            rgb_set_cab_color(0, rgb32(cdc->data.body, cdc->data.body, cdc->data.body, false));
            rgb_set_cab_color(1, rgb32(cdc->data.ext, cdc->data.ext, cdc->data.ext, false));
            rgb_set_cab_color(2, rgb32(cdc->data.side, cdc->data.side, cdc->data.side, false));
            break;
        case 0x3C:
            printf("Upd\n");
            break;
        case 0x82:
            printf("Dir\n");
            break;
        default:
            printf("LEDUnk\n");
            return;
    }
}

static inline void assemble_cmd(cdc_t *cdc, uint8_t c)
{
    if (c == 0xE0) {
        cdc->len = 0;
        cdc->in_cmd = true;
        cdc->is_touch = false;
        cdc->escape = false;
        cdc->checksum = 0;
        return;
    }

    if ((!cdc->in_cmd || cdc->is_touch) && (c == '{')) {
        cdc->len = 0;
        cdc->in_cmd = true;
        cdc->is_touch = true;
        return;
    }

    if (cdc->is_touch) {
        // Touch cmd
        if (c == '}') {
            touch_cmd(cdc);
        } else if (cdc->len < sizeof(cdc->buf)) {
            cdc->buf[cdc->len] = c;
            cdc->len++;
        }
        return;
    }

    // LED cmd
    if (c == 0xD0) {
        cdc->escape = true;
        return;
    }
    
    if (cdc->escape) {
        cdc->escape = false;
        c++;
    }

    if ((cdc->len == cdc->hdr.len + 3) && (cdc->checksum == c)) {
        led_cmd(cdc);
        return;
    }
    
    if (cdc->len < sizeof(cdc->buf)) {
        cdc->buf[cdc->len] = c;
        cdc->len++;
        cdc->checksum += c;
    }
}

static void update_itf(cdc_t *cdc)
{
    cdc->connected = tud_cdc_n_connected(cdc->interface);

    if (tud_cdc_n_available(cdc->interface)) {
        uint8_t buf[48];
        uint32_t count = tud_cdc_n_read(cdc->interface, buf, sizeof(buf));
        for (int i = 0; i < count; i++) {
            assemble_cmd(cdc, buf[i]);
        }
    }
}

static void send_touch()
{
    if ((ctx.touch_interface == 0) | (!ctx.stat)) {
        return;
    }

    static uint64_t last_sent_time = 0;
    uint64_t now = time_us_64();
    if (now - last_sent_time < 1000) {
        return;
    }
    last_sent_time = now;


    uint8_t report[9] = "(\0\0\0\0\0\0\0)";
    uint64_t touch = touch_touchmap();
    for (int i = 0; i < 7; i++) {
        report[i + 1] = touch & 0x1f;
        touch >>= 5;
    }
    tud_cdc_n_write(ctx.touch_interface, report, sizeof(report));
    tud_cdc_n_write_flush(ctx.touch_interface);
}

void io_update()
{
    update_itf(cdc);
    update_itf(cdc + 1);
    send_touch();
}

uint64_t io_last_io_time()
{
    return ctx.last_io_time;
}
