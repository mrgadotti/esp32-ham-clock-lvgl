#include "display.h"

#include <TFT_eSPI.h>
#include <lvgl.h>

#include "board.h"
#include "app/config_store.h"

static TFT_eSPI tft;
static lv_display_t* disp;
// One 320x20 RGB565 partial buffer (12.8 KB), heap-allocated: static DRAM that large starves the global ctors.
static constexpr uint32_t BUF_PX = 320 * 20;

#ifdef PERF_TEST
uint32_t g_perfFlushes, g_perfFlushUs, g_perfPixels, g_perfFrames;
#endif

static void flushCb(lv_display_t* d, const lv_area_t* a, uint8_t* px) {
#ifdef PERF_TEST
  uint32_t t0 = micros();
#endif
  uint32_t w = a->x2 - a->x1 + 1, h = a->y2 - a->y1 + 1;
  tft.startWrite();
  tft.setAddrWindow(a->x1, a->y1, w, h);
  tft.pushColors((uint16_t*)px, w * h, true);  // true = swap bytes for LVGL's native order
  tft.endWrite();
#ifdef PERF_TEST
  g_perfFlushUs += micros() - t0;
  g_perfFlushes++;
  g_perfPixels += w * h;
  if (lv_display_flush_is_last(d)) g_perfFrames++;
#endif
  lv_display_flush_ready(d);
}

void displaySetBacklight(uint8_t percent) {
  percent = constrain(percent, 5, 100);
  ledcWrite(0, (uint32_t)percent * 255 / 100);
}

void displayApplyNightDim(bool night) {
  int bl = cfg().tft_backlight, nd = cfg().tft_ndim;
  displaySetBacklight(night && nd > 0 ? bl * (100 - nd) / 100 : bl);  // displaySetBacklight clamps to >= 5
}

void displaySetInvert(bool invert) {
  tft.invertDisplay(invert);
}

void displaySetRotation(uint8_t rot) {
  tft.setRotation(rot == 3 ? 3 : 1);
  if (disp) lv_obj_invalidate(lv_screen_active());  // repaint whole screen with the new scan direction
}

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
  void* buf1 = heap_caps_malloc(BUF_PX * 2, MALLOC_CAP_8BIT);
  void* buf2 = nullptr;  // single buffer: frees 12.8 KB of contiguous heap for TLS
  lv_display_set_buffers(disp, buf1, buf2, BUF_PX * 2, LV_DISPLAY_RENDER_MODE_PARTIAL);
}
