#include "display.h"

#include <TFT_eSPI.h>
#include <lvgl.h>

#include "board.h"

static TFT_eSPI tft;
static lv_display_t* disp;
// Two 320x10 RGB565 partial buffers (12.8 KB total).
static uint16_t buf1[320 * 10] __attribute__((aligned(4)));
static uint16_t buf2[320 * 10] __attribute__((aligned(4)));

static void flushCb(lv_display_t* d, const lv_area_t* a, uint8_t* px) {
  uint32_t w = a->x2 - a->x1 + 1, h = a->y2 - a->y1 + 1;
  tft.startWrite();
  tft.setAddrWindow(a->x1, a->y1, w, h);
  tft.pushColors((uint16_t*)px, w * h, true);  // true = swap bytes for LVGL's native order
  tft.endWrite();
  lv_display_flush_ready(d);
}

void displaySetBacklight(uint8_t percent) {
  percent = constrain(percent, 5, 100);
  ledcWrite(0, (uint32_t)percent * 255 / 100);
}

void displaySetInvert(bool invert) { tft.invertDisplay(invert); }

void displaySetRotation(uint8_t rot) { tft.setRotation(rot == 3 ? 3 : 1); }

void displayInit(uint8_t rotation, bool invert) {
  ledcSetup(0, 5000, 8);
  ledcAttachPin(PIN_BL, 0);
  displaySetBacklight(100);

  tft.init();
  displaySetRotation(rotation);
  displaySetInvert(invert);
  tft.fillScreen(TFT_BLACK);

  lv_init();
  lv_tick_set_cb([]() -> uint32_t { return millis(); });
  disp = lv_display_create(320, 240);
  lv_display_set_flush_cb(disp, flushCb);
  lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
}
