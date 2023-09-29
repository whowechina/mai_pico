/*
 * RGB LED (WS2812) Strip control
 * WHowe <github.com/whowechina>
 * 
 */

#include "rgb.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "bsp/board.h"
#include "hardware/pio.h"
#include "hardware/timer.h"

#include "ws2812.pio.h"

#include "board_defs.h"
#include "config.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static uint32_t rgb_buf[20];

#define _MAP_LED(x) _MAKE_MAPPER(x)
#define _MAKE_MAPPER(x) MAP_LED_##x
#define MAP_LED_RGB { c1 = r; c2 = g; c3 = b; }
#define MAP_LED_GRB { c1 = g; c2 = r; c3 = b; }

#define REMAP_BUTTON_RGB _MAP_LED(BUTTON_RGB_ORDER)
#define REMAP_TT_RGB _MAP_LED(TT_RGB_ORDER)

static inline uint32_t _rgb32(uint32_t c1, uint32_t c2, uint32_t c3, bool gamma_fix)
{
    if (gamma_fix) {
        c1 = ((c1 + 1) * (c1 + 1) - 1) >> 8;
        c2 = ((c2 + 1) * (c2 + 1) - 1) >> 8;
        c3 = ((c3 + 1) * (c3 + 1) - 1) >> 8;
    }
    
    return (c1 << 16) | (c2 << 8) | (c3 << 0);    
}

uint32_t rgb32(uint32_t r, uint32_t g, uint32_t b, bool gamma_fix)
{
#if BUTTON_RGB_ORDER == GRB
    return _rgb32(g, r, b, gamma_fix);
#else
    return _rgb32(r, g, b, gamma_fix);
#endif
}

uint32_t rgb32_from_hsv(uint8_t h, uint8_t s, uint8_t v)
{
    uint32_t region, remainder, p, q, t;

    if (s == 0) {
        return v << 16 | v << 8 | v;
    }

    region = h / 43;
    remainder = (h % 43) * 6;

    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0:
            return v << 16 | t << 8 | p;
        case 1:
            return q << 16 | v << 8 | p;
        case 2:
            return p << 16 | v << 8 | t;
        case 3:
            return p << 16 | q << 8 | v;
        case 4:
            return t << 16 | p << 8 | v;
        default:
            return v << 16 | p << 8 | q;
    }
}

static void drive_led()
{
    static uint64_t last = 0;
    uint64_t now = time_us_64();
    if (now - last < 4000) { // no faster than 250Hz
        return;
    }
    last = now;

    for (int i = 0; i < ARRAY_SIZE(rgb_buf); i++) {
        pio_sm_put_blocking(pio0, 0, rgb_buf[i] << 8u);
    }
}

void rgb_set_colors(const uint32_t *colors, unsigned index, size_t num)
{
    if (index >= ARRAY_SIZE(rgb_buf)) {
        return;
    }
    if (index + num > ARRAY_SIZE(rgb_buf)) {
        num = ARRAY_SIZE(rgb_buf) - index;
    }
    memcpy(&rgb_buf[index], colors, num * sizeof(*colors));
}

static inline uint32_t apply_level(uint32_t color)
{
    unsigned r = (color >> 16) & 0xff;
    unsigned g = (color >> 8) & 0xff;
    unsigned b = color & 0xff;

    r = r * mai_cfg->style.level / 255;
    g = g * mai_cfg->style.level / 255;
    b = b * mai_cfg->style.level / 255;

    return r << 16 | g << 8 | b;
}

void rgb_set_color(unsigned index, uint32_t color)
{
    if (index >= ARRAY_SIZE(rgb_buf)) {
        return;
    }
    rgb_buf[index] = apply_level(color);
}

void rgb_set_brg(unsigned index, const uint8_t *brg_array, size_t num)
{
    if (index >= ARRAY_SIZE(rgb_buf)) {
        return;
    }
    if (index + num > ARRAY_SIZE(rgb_buf)) {
        num = ARRAY_SIZE(rgb_buf) - index;
    }
    for (int i = 0; i < num; i++) {
        uint8_t b = brg_array[i * 3 + 0];
        uint8_t r = brg_array[i * 3 + 1];
        uint8_t g = brg_array[i * 3 + 2];
        rgb_buf[index + i] = apply_level(rgb32(r, g, b, false));
    }
}

void rgb_init()
{
    uint pio0_offset = pio_add_program(pio0, &ws2812_program);

    gpio_set_drive_strength(RGB_PIN, GPIO_DRIVE_STRENGTH_2MA);
    ws2812_program_init(pio0, 0, pio0_offset, RGB_PIN, 800000, false);
}

void rgb_update()
{
    drive_led();
}

#if 0


#if defined(__AVR_ATmega32U4__) || defined(ARDUINO_SAMD_ZERO)
#pragma message "当前的开发板是 ATmega32U4 或 SAMD_ZERO"
#define SerialDevice SerialUSB
#define LED_PIN 13

#elif defined(ARDUINO_ESP8266_NODEMCU_ESP12E)
#pragma message "当前的开发板是 NODEMCU_ESP12E"
#define SerialDevice Serial
#define LED_PIN D5

#elif defined(ARDUINO_NodeMCU_32S)
#pragma message "当前的开发板是 NodeMCU_32S"
#define SerialDevice Serial
#define LED_PIN 13


#else
#error "未经测试的开发板，请检查串口和阵脚定义"
#endif

#include "FastLED.h"
#define NUM_LEDS 11
CRGBArray<NUM_LEDS> leds;


enum {
  LedGs8Bit = 0x31,//49
  LedGs8BitMulti = 0x32,//50
  LedGs8BitMultiFade = 0x33,//51
  LedFet = 0x39,//57
  LedGsUpdate = 0x3C,//60
  LedDirect = 0x82,
};

typedef union {
  uint8_t base[64];
  struct {
    struct {
      uint8_t dstNodeID;
      uint8_t srcNodeID;
      uint8_t length;
      uint8_t cmd;
    };
    union {
      struct { //39
        uint8_t color[11][3];//CRGB
      };
      struct { //9
        uint8_t BodyLed;
        uint8_t ExtLed;
        uint8_t SideLed;
      };
      struct { //LedGs8Bit
        uint8_t index;
        uint8_t r;
        uint8_t g;
        uint8_t b;
      };
      struct { //LedGs8BitMulti,LedGs8BitMultiFade
        uint8_t start;
        uint8_t end;//length
        uint8_t skip;
        uint8_t mr;
        uint8_t mg;
        uint8_t mb;
        uint8_t speed;
      };
    };
  };
} Packet;

static Packet req;

static uint8_t len, r, checksum;
static bool escape = false;

static uint8_t packet_read() {
  while (SerialDevice.available()) {
    r = SerialDevice.read();
    if (r == 0xE0) {
      len = 0;
      checksum = 0;
      continue;
    }
    if (r == 0xD0) {
      escape = true;
      continue;
    }
    if (escape) {
      r++;
      escape = false;
    }

    if (len - 3 == req.length && checksum == r) {
      return req.cmd;
    }
    req.base[len++] = r;
    checksum += r;
  }
  return 0;
}

void setup() {
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(30);
  FastLED.clear();
  SerialDevice.begin(115200);
  leds(0, 7) = 0xFFFFFF;
  FastLED.delay(1000);
}

unsigned long fade_start, fade_end, progress;
uint8_t led_start, led_end, fade_tag;
CRGB fade_prev, fade_taget;

void loop() {
  switch (packet_read()) {
    case LedGs8Bit:
      leds[req.index] = CRGB(req.r, req.g, req.b);
      break;

    case LedGs8BitMulti:
      if (req.end == 0x20) {
        req.end = NUM_LEDS;
      }
      leds(req.start, req.end - 1) = CRGB(req.mr, req.mg, req.mb);
      fade_prev = CRGB(req.mr, req.mg, req.mb);
      fade_tag = 0;
      break;

    case LedGs8BitMultiFade:
      fade_taget = CRGB(req.mr, req.mg, req.mb);
      fade_start = millis();
      fade_end = fade_start + (4095 / req.speed * 8);
      led_start = req.start;
      led_end = req.end - 1;
      fade_tag = 1;
      break;

    case LedFet://框体灯，只有白色，值代表亮度，会多次发送实现渐变，需要立刻刷新
      leds[8] = blend(0x000000, 0xFFFFFF, req.BodyLed);
      leds[9] = blend(0x000000, 0xFFFFFF, req.ExtLed);//same as BodyLed
      leds[10] = blend(0x000000, 0xFFFFFF, req.SideLed);//00 or FF
      FastLED.show();
      break;

    case LedGsUpdate://提交灯光数据
      FastLED.show();
      break;
  }

  if (!fade_tag)return;
  if (millis() > fade_end) {
    progress = 255;
    fade_tag = 0;
  } else {
    progress = map(millis(), fade_start, fade_end, 0, 255);
  }
  leds(led_start, led_end) = blend(fade_prev, fade_taget, progress);
  FastLED.show();
}

#endif