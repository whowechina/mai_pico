/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "usb_descriptors.h"
#include "pico/unique_id.h"
#include "tusb.h"

tusb_desc_device_t desc_device_joy = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor = 0x1f2d,
    .idProduct = 0x0123,
    .bcdDevice = 0x0100,

    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,

    .bNumConfigurations = 0x01
};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const* tud_descriptor_device_cb(void) {
    return (uint8_t const*)&desc_device_joy;
}

//--------------------------------------------------------------------+
// HID Report Descriptor
//--------------------------------------------------------------------+

uint8_t const desc_hid_report_joy[] = {
    MAIPICO_REPORT_DESC_JOYSTICK,
};

uint8_t const desc_hid_report_nkro[] = {
    MAIPICO_REPORT_DESC_NKRO,
};

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const* tud_hid_descriptor_report_cb(uint8_t itf)
{
    switch (itf) {
        case 0:
            return desc_hid_report_joy;
        case 1:
            return desc_hid_report_nkro;
        default:
            return NULL;
    }
}
//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

enum { ITF_NUM_JOY, ITF_NUM_NKRO,
       ITF_NUM_CDC_CLI, ITF_NUM_CDC_CLI_DATA,
       ITF_NUM_CDC_TOUCH, ITF_NUM_CDC_TOUCH_DATA,
       ITF_NUM_CDC_LED, ITF_NUM_CDC_LED_DATA,
       ITF_NUM_TOTAL };

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN * 2 + TUD_CDC_DESC_LEN * 3)

#define EPNUM_JOY 0x81
#define EPNUM_KEY 0x82

#define EPNUM_CDC_CLI_NOTIF 0x83
#define EPNUM_CDC_CLI_OUT   0x04
#define EPNUM_CDC_CLI_IN    0x84

#define EPNUM_CDC_TOUCH_NOTIF 0x85
#define EPNUM_CDC_TOUCH_OUT 0x06
#define EPNUM_CDC_TOUCH_IN  0x86
#define EPNUM_CDC_LED_NOTIF 0x87
#define EPNUM_CDC_LED_OUT 0x08
#define EPNUM_CDC_LED_IN  0x88


uint8_t const desc_configuration_joy[] = {
    // Config number, interface count, string index, total length, attribute,
    // power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 200),

    // Interface number, string index, protocol, report descriptor len, EP In
    // address, size & polling interval
    TUD_HID_DESCRIPTOR(ITF_NUM_JOY, 4, HID_ITF_PROTOCOL_NONE,
                       sizeof(desc_hid_report_joy), EPNUM_JOY,
                       CFG_TUD_HID_EP_BUFSIZE, 1),

    TUD_HID_DESCRIPTOR(ITF_NUM_NKRO, 5, HID_ITF_PROTOCOL_NONE,
                       sizeof(desc_hid_report_nkro), EPNUM_KEY,
                       CFG_TUD_HID_EP_BUFSIZE, 1),

    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_CLI, 6, EPNUM_CDC_CLI_NOTIF,
                       8, EPNUM_CDC_CLI_OUT, EPNUM_CDC_CLI_IN, 64),

    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_TOUCH, 7, EPNUM_CDC_TOUCH_NOTIF,
                       8, EPNUM_CDC_TOUCH_OUT, EPNUM_CDC_TOUCH_IN, 64),

    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_LED, 8, EPNUM_CDC_LED_NOTIF,
                       8, EPNUM_CDC_LED_OUT, EPNUM_CDC_LED_IN, 64),
};

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
    return desc_configuration_joy;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

static char serial_number_str[24] = "123456\0";

// array of pointer to string descriptors
static const char *string_desc_arr[] = {
    (const char[]){0x09, 0x04},  // 0: is supported language is English (0x0409)
    "WHowe"       ,              // 1: Manufacturer
    "Mai Pico Controller",       // 2: Product
    serial_number_str,           // 3: Serials, use chip ID
    "Mai Pico Joystick",
    "Mai Pico LED",
    "Mai Pico NKRO",
    "Mai Pico Command Serial Port",
    "Mai Pico Touch Serial Port",
    "Mai Pico LED Serial Port",
};

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long
// enough for transfer to complete
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    static uint16_t _desc_str[64];

    if (index == 0) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 + 2);
        return _desc_str;
    }
    
    if (index == 3) {
        pico_unique_board_id_t board_id;
        pico_get_unique_board_id(&board_id);
        sprintf(serial_number_str, "%016llx", *(uint64_t *)&board_id);
    }
    
    const size_t base_num = sizeof(string_desc_arr) / sizeof(string_desc_arr[0]);
    const char *colors[] = {"Blue", "Red", "Green"};
    char str[64];

    if (index < base_num) {
        strcpy(str, string_desc_arr[index]);
    } else if (index < base_num + 48 + 45) {
        const char *names[] = {"Key ", "Splitter "};
        int led = index - base_num;
        int id = led / 6 + 1;
        int type = led / 3 % 2;
        int brg = led % 3;
        sprintf(str, "%s%02d %s", names[type], id, colors[brg]);
    } else if (index < base_num + 48 + 45 + 18) {
        int led = index - base_num - 48 - 45;
        int id = led / 3 + 1;
        int brg = led % 3;
        sprintf(str, "Tower %02d %s", id, colors[brg]);
    } else {
        sprintf(str, "Unknown %d", index);
    }

    uint8_t chr_count = strlen(str);
    if (chr_count > 63) {
        chr_count = 63;
    }

    // Convert ASCII string into UTF-16
    for (uint8_t i = 0; i < chr_count; i++) {
        _desc_str[1 + i] = str[i];
    }

    // first byte is length (including header), second byte is string type
    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);

    return _desc_str;
}
