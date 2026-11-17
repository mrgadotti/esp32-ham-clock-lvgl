#include "display.h"

#include <TFT_eSPI.h>
#include <lvgl.h>

#include "board.h"
#include "app/config_store.h"
#ifdef SCREEN_TEST
#include "debug/screen_test.h"
#endif

static TFT_eSPI tft;
static lv_display_t* disp;
// Two 320x20 RGB565 partial buffers (25.6 KB total), heap-allocated: static DRAM that large starves the global ctors.
// One is rendered while the other goes out over SPI by DMA.
static constexpr uint32_t BUF_PX = 320 * 20;

#ifdef PERF_TEST
uint32_t g_perfFlushes, g_perfFlushUs, g_perfPixels, g_perfFrames;
#endif

static bool dmaPending;  // a chunk is on the wire and the transaction is still open

static void flushCb(lv_display_t* d, const lv_area_t* a, uint8_t* px) {
#ifdef PERF_TEST
  uint32_t t0 = micros();
#endif
#ifdef SCREEN_TEST
  if (screenTestCapturing()) screenTestFlush(a, px, lv_display_flush_is_last(d));
#endif
  uint32_t w = a->x2 - a->x1 + 1, h = a->y2 - a->y1 + 1;
  tft.startWrite();  // CS stays low until flushWaitCb: the DMA is still running when this returns
  tft.setAddrWindow(a->x1, a->y1, w, h);
  tft.pushPixelsDMA((uint16_t*)px, w * h);  // buffers are RGB565_SWAPPED: already in panel byte order
  dmaPending = true;
#ifdef PERF_TEST
  g_perfFlushUs += micros() - t0;
  g_perfFlushes++;
  g_perfPixels += w * h;
  if (lv_display_flush_is_last(d)) g_perfFrames++;
#endif
}

static void flushWaitCb(lv_display_t* d) {
  if (dmaPending) {
    tft.dmaWait();
    tft.endWrite();
    dmaPending = false;
  }
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
  if (disp) flushWaitCb(disp);  // let the running DMA chunk finish before other panel commands
  tft.invertDisplay(invert);
}

void displaySetRotation(uint8_t rot) {
  if (disp) flushWaitCb(disp);
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
  tft.initDMA();

  lv_init();
  lv_tick_set_cb([]() -> uint32_t { return millis(); });
  disp = lv_display_create(320, 240);
  lv_display_set_flush_cb(disp, flushCb);
  lv_display_set_flush_wait_cb(disp, flushWaitCb);
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565_SWAPPED);
  void* buf1 = heap_caps_malloc(BUF_PX * 2, MALLOC_CAP_8BIT);
  void* buf2 = heap_caps_malloc(BUF_PX * 2, MALLOC_CAP_8BIT);
  lv_display_set_buffers(disp, buf1, buf2, BUF_PX * 2, LV_DISPLAY_RENDER_MODE_PARTIAL);
}
