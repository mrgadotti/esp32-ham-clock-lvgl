#include <Arduino.h>
#include <lvgl.h>

#include "app/config_store.h"
#include "app/data_store.h"
#include "app/events.h"
#include "app/mem_log.h"
#include "hal/display.h"
#include "hal/touch.h"
#include "hal/touch_cal_ui.h"
#include "net/time_service.h"
#include "net/web_config.h"
#include "net/wifi_manager.h"
#include "svc/dx_cluster.h"
#include "svc/hamalert.h"
#include "svc/pota.h"
#include "svc/qrz.h"
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

static void uiTask(void*) {
  cfgRefreshUiSnapshot();
  applyCfgToHal();
  if (!touchCalLoadSaved() || touchHeldAtBoot(2000)) touchCalRun(nullptr);
  uiInit();
  uint32_t lastMem = 0;
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
    lv_timer_handler();
    uint32_t now = millis();
    uiTick(now);
    if (now - lastMem >= 1000) {
      lastMem = now;
      lv_mem_monitor_t m;
      lv_mem_monitor(&m);
      memLogSetLvFree(m.free_size);
    }
    vTaskDelay(pdMS_TO_TICKS(5));
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
    potaTick(now);
    hamalertTick(now);
    qrzTick(now);
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
  TaskHandle_t hUi, hNet;
  xTaskCreatePinnedToCore(uiTask, "ui", 8192, nullptr, 3, &hUi, 1);
  xTaskCreatePinnedToCore(netTask, "net", 10240, nullptr, 1, &hNet, 0);
  memLogSetTasks(hNet, hUi);
  xTaskCreatePinnedToCore(uartTask, "uart", 4096, nullptr, 2, nullptr, 1);
}

void loop() { vTaskDelete(NULL); }
