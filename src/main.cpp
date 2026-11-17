#include <Arduino.h>
#include <lvgl.h>

#include "app/boot_log.h"
#include "app/config_store.h"
#include "app/data_store.h"
#include "app/events.h"
#include "app/mem_log.h"
#include "hal/display.h"
#include "hal/rgb_led.h"
#include "hal/touch.h"
#include "hal/touch_cal_ui.h"
#ifdef SCREEN_TEST
#include "debug/screen_test.h"
#endif
#include "net/time_service.h"
#include "net/web_config.h"
#include "net/wifi_manager.h"
#include "svc/aprs_ingest.h"
#include "svc/aprs_is.h"
#include "svc/aprs_beacon.h"
#include "svc/dx_cluster.h"
#include "svc/hamalert.h"
#include "svc/pota.h"
#include "svc/propagation.h"
#include "svc/qrz.h"
#include "svc/uart2.h"
#include "svc/weather.h"
#include "ui/fonts/fonts.h"
#include "ui/theme.h"
#include "ui/ui_manager.h"
#ifdef PERF_TEST
#include "perf_prof.h"
#include "ui/widgets/spot_table.h"
#endif

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
static lv_obj_t* perfFind() {  // the scrollable child of the active screen (Settings list or the spot table)
  lv_obj_t* scr = lv_screen_active();
  for (uint32_t i = 0; i < lv_obj_get_child_count(scr); i++) {
    lv_obj_t* c = lv_obj_get_child(scr, i);
    if (lv_obj_has_flag(c, LV_OBJ_FLAG_SCROLLABLE) && lv_obj_get_scroll_bottom(c) + lv_obj_get_scroll_top(c) > 0) return c;
  }
  return nullptr;
}
static bool perfCell(size_t r, uint8_t c, char* out, size_t cap, void*) {
  static const char* const k[] = {"1234", "DL1ABC/P", "14.074", "DIGI", "Germany fake"};
  if (c == 0) snprintf(out, cap, "%04u", (unsigned)(1200 + r));
  else snprintf(out, cap, "%s", k[c]);
  return true;
}
static void perfInject() {  // fake rows into the spot table; real list refreshes (empty store) hide it again, so repeat
  for (uint32_t i = 0; i < lv_obj_get_child_count(lv_screen_active()); i++) {
    lv_obj_t* c = lv_obj_get_child(lv_screen_active(), i);
    if (lv_obj_get_user_data(c) && lv_obj_has_flag(c, LV_OBJ_FLAG_SCROLLABLE)) {
      lv_obj_remove_flag(c, LV_OBJ_FLAG_HIDDEN);
      spotTableSet(c, 100, perfCell, nullptr);
    }
  }
}
static void perfScrollCb(lv_timer_t*) {
  static int dir = 1;
  lv_obj_t* l = perfFind();
  if (!l) { perfInject(); return; }
  if (dir > 0 && lv_obj_get_scroll_bottom(l) <= 0) dir = -1;
  if (dir < 0 && lv_obj_get_scroll_top(l) <= 0) dir = 1;
  lv_obj_scroll_by(l, 0, -dir * 6, LV_ANIM_OFF);
}
static void perfReport() {
#ifdef PERF_PROF
  perfProfDump();
#endif
  Serial.printf("PERF fps=%u flushes=%u flush_ms=%u px=%u handler_calls=%u handler_max_ms=%u handler_tot_ms=%u\n", g_perfFrames, g_perfFlushes,
                g_perfFlushUs / 1000, g_perfPixels, perfHandlerCalls, perfHandlerMaxUs / 1000, perfHandlerTotUs / 1000);
  g_perfFrames = g_perfFlushes = g_perfFlushUs = g_perfPixels = perfHandlerCalls = perfHandlerMaxUs = perfHandlerTotUs = 0;
}
#endif

static void uiTask(void*) {
  cfgRefreshUiSnapshot();
  applyCfgToHal();
  bool calOk = touchCalLoadSaved();
  bootLog(calOk ? "Touch calibration loaded" : "Touch calibration needed");
  if (!calOk || touchHeldAtBoot(2000)) touchCalRun(nullptr);
  uiInit();
  uint32_t lastMem = 0;
#ifdef PERF_TEST
  uiRequest(ScreenId::Settings);
  uint32_t perfStart = 0, perfLast = 0;
  lv_timer_t* perfTimer = nullptr;
  int perfPhase = 0;
#endif
  for (;;) {
#ifdef SCREEN_TEST
    screenTestPoll();
#endif
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
    uint32_t wait = lv_timer_handler();  // ms until LVGL needs to run again
#ifdef PERF_TEST
    uint32_t pd = micros() - pt0;
    perfHandlerCalls++;
    perfHandlerTotUs += pd;
    if (pd > perfHandlerMaxUs) perfHandlerMaxUs = pd;
    uint32_t pn = millis();
    if (perfPhase == 0 && pn > 15000) { perfPhase = 5; perfStart = pn; uiRequest(ScreenId::Settings); }  // boot screen eats earlier requests
    if (perfPhase == 5 && pn - perfStart >= 1500) { perfPhase = 1; perfLast = perfStart = pn; perfReport(); perfTimer = lv_timer_create(perfScrollCb, 16, nullptr); Serial.printf("PERF settings cpu=%u MHz\n", getCpuFrequencyMhz()); }
    if (perfPhase == 1 && pn - perfStart >= 10000) {
      perfPhase = 2; perfStart = pn; lv_timer_delete(perfTimer); perfTimer = nullptr; uiRequest(ScreenId::DxCluster);
    }
    if (perfPhase == 2 && pn - perfStart >= 1500) {  // screen built and its empty first refresh done: inject fake rows
      perfInject();
      perfPhase = 3; perfStart = perfLast = pn; perfReport(); perfTimer = lv_timer_create(perfScrollCb, 16, nullptr); Serial.println("PERF dx table");
    }
    if (perfPhase == 1 || perfPhase == 3) {
      if (pn - perfLast >= 1000) { perfLast = pn; perfReport(); }
      if (perfPhase == 3 && pn - perfStart >= 10000) { lv_timer_delete(perfTimer); perfTimer = nullptr; perfPhase = 4; Serial.println("PERF done"); }
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
    vTaskDelay(pdMS_TO_TICKS(wait < 1 ? 1 : wait > 5 ? 5 : wait));  // cap: touch and events are polled between LVGL timers
  }
}

// Heap order after WiFi comes up (M-06): TLS needs ~2 x 16.6 KB of contiguous blocks per handshake, and every open
// socket pins small blocks in the middle of that region. So the HTTPS services run their first round before the
// socket services (DX, APRS-IS, HamAlert) may connect, plus a short settle time.
constexpr uint32_t kSocketDelayMs = 3000;   // after the first HTTPS round
constexpr uint32_t kRoundFallbackMs = 15000;  // first round is declared done anyway (no clock, no weather key...)

// One boot-log line with the services the config enables (no hosts or credentials), then the heap state.
static void logServices() {
  char b[48] = "Svc: POTA PROP";  // no config switch: always on
  auto add = [&](bool on, const char* n) { if (on) { strlcat(b, " ", sizeof b); strlcat(b, n, sizeof b); } };
  cfgRead([&](const Config& c) {
    add(c.cluster_host[0], "DX");
    add(c.aprs_host[0] && c.user_callsign[0], "APRS");
    add(c.hama_host[0] && c.hama_user[0] && c.hama_pass[0], "HAMA");
    add(c.weather_key[0], "WX");
    add(c.qrz_user[0] && c.qrz_pass[0], "QRZ");
  });
  bootLog("%s", b);
  bootLog("Heap free %u max %u", (unsigned)ESP.getFreeHeap(), (unsigned)memLargest8());
}

static void netTask(void*) {
  webInit();
  uint32_t staAt = 0, roundAt = 0;
  for (;;) {
    uint32_t now = millis();
    bool sta = false;
    g_store.net.read([&](const NetStatus& s) { sta = s.sta; });
    if (!sta) staAt = roundAt = 0;
    else if (!staAt) staAt = now | 1;
    wifiTick(now);
    webTick();
    timeTick(now);
    potaTick(now);
    qrzTick(now);
    weatherTick(now);
    propTick(now);
    // The tick that sees a valid clock also ran the first propagation fetch, so the round is over at the end of it.
    now = millis();  // the HTTPS fetches above can take seconds
    if (sta && !roundAt && (timeValid() || now - staAt >= kRoundFallbackMs)) {
      roundAt = now | 1;
      logServices();
      g_store.net.write([](NetStatus& s) { s.roundDone = true; });  // releases the boot screen (ui_manager)
    }
    if (!sta || (roundAt && now - roundAt >= kSocketDelayMs)) {  // !sta: let the services close their sockets
      dxTick(now);
      aprsIsTick(now);
      hamalertTick(now);
    }
    aprsBeaconTick(now);
    rgbLedTick(now);
    memLogTick(now);
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void setup() {
  dataStoreInit();
#ifdef SCREEN_TEST
  Serial.begin(SCREEN_TEST_BAUD);
  screenTestInit();
#else
  Serial.begin(115200);
#endif
  bootLog("ESP32-HAM-CLOCK %s", FW_VERSION);
  Serial.printf("[MEM] boot largest=%u free=%u\n", (unsigned)memLargest8(), (unsigned)ESP.getFreeHeap());
  Config boot;
  configStoreLoad(boot);
  bootLog("Config: %s, %s", boot.user_callsign, boot.tft_lang ? "PT-BR" : "EN");
  displayInit(boot.tft_rot, boot.tft_inv);
  bootLog("Display 320x240 rot %d inv %d", (int)boot.tft_rot, (int)boot.tft_inv);
  displaySetBacklight(boot.tft_backlight);
  Serial.printf("[MEM] after display largest=%u free=%u\n", (unsigned)memLargest8(), (unsigned)ESP.getFreeHeap());
  themeApply(lv_display_get_default());
  touchInit();
  touchCreateIndev();
  bootLog("Touch ready");
  eventsInit();
  qrzInit();
  aprsIngestInit();
  TaskHandle_t hUi, hNet, hUart;
  xTaskCreatePinnedToCore(uiTask, "ui", 8192, nullptr, 3, &hUi, 1);
  xTaskCreatePinnedToCore(netTask, "net", 8192, nullptr, 1, &hNet, 0);
  xTaskCreatePinnedToCore(uart2Task, "uart", 6144, nullptr, 2, &hUart, 1);
  memLogSetTasks(hNet, hUi, hUart);
}

void loop() { vTaskDelete(NULL); }
