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
#include "net/wifi_manager.h"
#include "ui/fonts/fonts.h"
#include "ui/theme.h"

static lv_obj_t *lblWifi, *lblSsid, *lblIp, *lblRssi, *lblTime, *lblMem;

static void buildStatusScreen() {
  lv_obj_t* scr = lv_screen_active();
  lv_obj_t** labels[] = {&lblWifi, &lblSsid, &lblIp, &lblRssi, &lblTime, &lblMem};
  int y = 8;
  for (auto l : labels) {
    *l = lv_label_create(scr);
    lv_obj_set_style_text_font(*l, &font_14, 0);
    lv_obj_set_pos(*l, 8, y);
    y += 22;
  }
}

static void refreshStatus() {
  NetStatus n{};
  g_store.net.read([&](const NetStatus& s) { n = s; });
  lv_label_set_text_fmt(lblWifi, "WiFi: %s", n.sta ? "STA" : (n.ap ? "AP" : "connecting"));
  lv_label_set_text_fmt(lblSsid, "SSID: %s", n.ssid);
  lv_label_set_text_fmt(lblIp, "IP: %s", n.ip);
  lv_label_set_text_fmt(lblRssi, "RSSI: %d", n.rssi);
  if (timeValid()) {
    struct tm tm;
    time_t t = time(nullptr);
    gmtime_r(&t, &tm);
    lv_label_set_text_fmt(lblTime, "UTC: %04d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1,
                          tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  } else {
    lv_label_set_text(lblTime, "UTC: waiting for NTP");
  }
  lv_label_set_text_fmt(lblMem, "[MEM] free=%u min=%u lv=%u", (unsigned)ESP.getFreeHeap(),
                        (unsigned)ESP.getMinFreeHeap(), (unsigned)memLogLvFree());
}

static void uiTask(void*) {
  cfgRefreshUiSnapshot();
  buildStatusScreen();
  if (!touchCalLoadSaved() || touchHeldAtBoot(2000)) touchCalRun(nullptr);
  uint32_t lastStatus = 0;
  for (;;) {
    UiEvent e;
    while (popUiEvent(e)) {
      if (e.kind == EventKind::ConfigChanged) cfgRefreshUiSnapshot();
      else if (e.kind == EventKind::StartTouchCal) touchCalRun(nullptr);
    }
    lv_timer_handler();
    uint32_t now = millis();
    if (now - lastStatus >= 1000) {
      lastStatus = now;
      lv_mem_monitor_t m;
      lv_mem_monitor(&m);
      memLogSetLvFree(m.free_size);
      refreshStatus();
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

static void netTask(void*) {
  for (;;) {
    uint32_t now = millis();
    wifiTick(now);
    timeTick(now);
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
  xTaskCreatePinnedToCore(uiTask, "ui", 8192, nullptr, 3, nullptr, 1);
  xTaskCreatePinnedToCore(netTask, "net", 10240, nullptr, 1, nullptr, 0);
  xTaskCreatePinnedToCore(uartTask, "uart", 4096, nullptr, 2, nullptr, 1);
}

void loop() { vTaskDelete(NULL); }
