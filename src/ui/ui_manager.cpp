#include "ui_manager.h"

#include <Arduino.h>
#include <atomic>

#include "i18n.h"
#include "../app/config_store.h"
#include "../app/data_store.h"
#include "../hal/display.h"
#include "../hal/touch.h"
#include "weather_json.h"
#include "../hal/touch_cal_ui.h"
#include "overlays/alert_overlay.h"
#include "screens/boot_screen.h"

static constexpr uint32_t TOUCH_PAUSE_MS = 10000, BOOT_MIN_MS = 3000, BOOT_MAX_MS = 10000;

static Screen* cur;
static lv_obj_t* curObj;
static std::atomic<int8_t> pendingDir{0};      // -1/+1 from gesture, uiNext/uiPrev
static std::atomic<uint8_t> pendingTarget{0};  // ScreenId, 0 = none
static bool booting;
static uint8_t lastLang;
static bool pendingRebuild;
static uint32_t bootStart, lastTick, lastSwitch;
static lv_obj_t* bootObj;
static bool night;
static uint32_t lastWxVer = ~0u;

static void applyNight() { displayApplyNightDim(night); }

static void onGesture(Gesture g) {  // indev callback: flag only
  pendingDir = (g == Gesture::SwipeLeft) ? 1 : -1;
}

void uiRequest(ScreenId id) { pendingTarget = (uint8_t)id; }
void uiNext() { pendingDir = 1; }
void uiPrev() { pendingDir = -1; }

void uiInit() {
  lastLang = cfg().tft_lang;
  i18nSetLang((Lang)lastLang);
  touchSetGestureSink(onGesture);
  bootObj = bootScreenCreate();
  lv_screen_load(bootObj);
  booting = true;
  bootStart = millis();
}

static uint32_t memShowAt;
static int memShowId;

static void showForce(ScreenId id) {
  lv_obj_t* obj = lv_obj_create(nullptr);
  Screen* s = createScreen(id);
  s->create(obj);
  lv_screen_load(obj);  // old screen is deleted below, after it is no longer active
  if (cur) delete cur;
  if (curObj) lv_obj_delete(curObj);
  if (bootObj) { lv_obj_delete(bootObj); bootObj = nullptr; }
  cur = s;
  curObj = obj;
  lastSwitch = millis();
  memShowAt = lastSwitch + 3000;  // let the screen populate before measuring
  memShowId = (int)id;
}

static void show(ScreenId id) {
  if (cur && cur->id() == id) return;
  showForce(id);
}

static ScreenId carouselNext(uint8_t from, int dir) {
  const Config& c = cfg();
  return (ScreenId)nextScreen(c.screen_order, (uint16_t)c.scr_en, from, dir);
}

void uiHandleEvent(const UiEvent& e) {
  switch (e.kind) {
    case EventKind::StartTouchCal: touchCalRun(nullptr); break;
    case EventKind::ConfigChanged:
      cfgRefreshUiSnapshot();
      applyNight();
      i18nSetLang((Lang)cfg().tft_lang);
      if (cfg().tft_lang != lastLang) {
        lastLang = cfg().tft_lang;
        if (cur) pendingRebuild = true;  // applied in uiTick
      }
      break;
    default: overlayShow(e); break;
  }
}

void uiTick(uint32_t now) {
  if (now - lastTick < 100) return;
  lastTick = now;

  overlayTick(now);
  if (memShowAt && now >= memShowAt) {
    memShowAt = 0;
    lv_mem_monitor_t m;
    lv_mem_monitor(&m);
    Serial.printf("[MEM] screen %d lvfree=%u\n", memShowId, (unsigned)m.free_size);
  }
  if (uint32_t v = g_store.weather.version(); v != lastWxVer) {
    lastWxVer = v;
    char ic[8];
    g_store.weather.read([&](const WeatherData& w) { memcpy(ic, w.iconCode, sizeof ic); });
    ic[7] = 0;
    night = isNightFromIcon(ic);
    applyNight();
  }
  if (booting) {
    NetStatus n;
    g_store.net.read([&](const NetStatus& s) { n = s; });
    bootScreenUpdate(n.sta);
    uint32_t el = now - bootStart;
    if (el < BOOT_MAX_MS && (el < BOOT_MIN_MS || !n.sta)) return;
    booting = false;
    pendingDir = 0;
    pendingTarget = 0;
    show(carouselNext(0, 1));
    return;
  }

  bool locked = overlayActive() || (cur && cur->blocksSwipe());
  int8_t dir = pendingDir.exchange(0);
  uint8_t target = pendingTarget.exchange(0);
  if (pendingRebuild) {
    pendingRebuild = false;
    showForce(cur->id());
  }
  if (target) {
    show((ScreenId)target);
  } else if (dir && !locked) {
    show(carouselNext((uint8_t)cur->id(), dir));
  } else if (cur && cfg().tft_autosw && !overlayActive() && cur->id() != ScreenId::Settings &&
             lv_display_get_inactive_time(nullptr) > TOUCH_PAUSE_MS &&
             now - lastSwitch >= (uint32_t)cfg().tft_sw_sec * 1000) {
    show(carouselNext((uint8_t)cur->id(), 1));
  }

  if (cur) cur->tick(now);
}
