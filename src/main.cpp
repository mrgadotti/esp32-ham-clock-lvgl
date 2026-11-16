#include <Arduino.h>
#include <lvgl.h>

#include "app/config_store.h"
#include "app/data_store.h"
#include "app/events.h"
#include "app/mem_log.h"
#include "hal/display.h"
#include "hal/rgb_led.h"
#include "hal/touch.h"
#include "hal/touch_cal_ui.h"
#include "net/time_service.h"
#include "net/web_config.h"
#include "net/wifi_manager.h"
#include "svc/aprs_ingest.h"
#include "svc/aprs_is.h"
#include "svc/dx_cluster.h"
#include "svc/hamalert.h"
#include "svc/pota.h"
#include "svc/propagation.h"
#include "svc/qrz.h"
#include "svc/weather.h"
#include "ui/fonts/fonts.h"
#include "ui/theme.h"
#include "ui/ui_manager.h"

static void applyCfgToHal() {
  static int32_t last = 0;
  if (cfg().tft_rot == last) return;
  last = cfg().tft_rot;
  touchSetMirror(last == 3);
  displaySetRotation(last);
}

#ifdef PERF_TEST
extern uint32_t g_perfFlushes, g_perfFlushUs, g_perfPixels, g_perfFrames;
static uint32_t perfHandlerCalls, perfHandlerMaxUs, perfHandlerTotUs;
static void perfScrollCb(lv_timer_t*) {
  static int dir = 1;
  lv_obj_t* l = lv_obj_get_child(lv_screen_active(), 1);
  if (!l) return;
  if (dir > 0 && lv_obj_get_scroll_bottom(l) <= 0) dir = -1;
  if (dir < 0 && lv_obj_get_scroll_top(l) <= 0) dir = 1;
  lv_obj_scroll_by(l, 0, -dir * 6, LV_ANIM_OFF);
}
static void perfReport() {
  Serial.printf("PERF fps=%u flushes=%u flush_ms=%u px=%u handler_calls=%u handler_max_ms=%u handler_tot_ms=%u\n", g_perfFrames, g_perfFlushes,
                g_perfFlushUs / 1000, g_perfPixels, perfHandlerCalls, perfHandlerMaxUs / 1000, perfHandlerTotUs / 1000);
  g_perfFrames = g_perfFlushes = g_perfFlushUs = g_perfPixels = perfHandlerCalls = perfHandlerMaxUs = perfHandlerTotUs = 0;
}
#endif

static void uiTask(void*) {
  cfgRefreshUiSnapshot();
  applyCfgToHal();
  if (!touchCalLoadSaved() || touchHeldAtBoot(2000)) touchCalRun(nullptr);
  uiInit();
  uint32_t lastMem = 0;
#ifdef PERF_TEST
  uiRequest(ScreenId::Settings);
  uint32_t perfStart = 0, perfLast = 0;
  lv_timer_t* perfTimer = nullptr;
  bool perfDone = false;
#endif
  for (;;) {
    UiEvent e;
    while (popUiEvent(e)) {
      uiHandleEvent(e);
      if (e.kind == EventKind::ConfigChanged) applyCfgToHal();
    }
    if (configDirtyTake()) {
      UiEvent c;
      c.kind = EventKind::ConfigChanged;
      uiHandleEvent(c);
      applyCfgToHal();
    }
#ifdef PERF_TEST
    uint32_t pt0 = micros();
#endif
    lv_timer_handler();
#ifdef PERF_TEST
    uint32_t pd = micros() - pt0;
    perfHandlerCalls++;
    perfHandlerTotUs += pd;
    if (pd > perfHandlerMaxUs) perfHandlerMaxUs = pd;
    uint32_t pn = millis();
    if (!perfTimer && !perfDone && pn > 4000) {
      perfTimer = lv_timer_create(perfScrollCb, 16, nullptr);
      perfStart = perfLast = pn;
      perfReport();
      Serial.println("PERF start scroll");
    }
    if (perfTimer && pn - perfLast >= 1000) {
      perfLast = pn;
      perfReport();
      if (pn - perfStart >= 10000) { lv_timer_delete(perfTimer); perfTimer = nullptr; perfDone = true; Serial.println("PERF done"); }
    }
#endif
    uint32_t now = millis();
    uiTick(now);
    if (now - lastMem >= 1000) {
      lastMem = now;
      lv_mem_monitor_t m;
      lv_mem_monitor(&m);
      memLogSetLvFree(m.free_size);
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

static void netTask(void*) {
  webInit();
  for (;;) {
    uint32_t now = millis();
    wifiTick(now);
    webTick();
    timeTick(now);
    dxTick(now);
    aprsIsTick(now);
    rgbLedTick(now);
    potaTick(now);
    hamalertTick(now);
    qrzTick(now);
    weatherTick(now);
    propTick(now);
    memLogTick(now);
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

static void uartTask(void*) {
  for (;;) vTaskDelay(pdMS_TO_TICKS(100));  // filled in by Task 17
}

void setup() {
  Serial.begin(115200);
  Config boot;
  configStoreLoad(boot);
  displayInit(boot.tft_rot, boot.tft_inv);
  displaySetBacklight(boot.tft_backlight);
  themeApply(lv_display_get_default());
  touchInit();
  touchCreateIndev();
  eventsInit();
  qrzInit();
  aprsIngestInit();
  TaskHandle_t hUi, hNet;
  xTaskCreatePinnedToCore(uiTask, "ui", 8192, nullptr, 3, &hUi, 1);
  xTaskCreatePinnedToCore(netTask, "net", 10240, nullptr, 1, &hNet, 0);
  memLogSetTasks(hNet, hUi);
  xTaskCreatePinnedToCore(uartTask, "uart", 4096, nullptr, 2, nullptr, 1);
}

void loop() { vTaskDelete(NULL); }
