// PERF_TEST harness (scroll / frame-rate measurement on the device). Compiled only with -DPERF_TEST.
#ifdef PERF_TEST
#include <Arduino.h>
#include <lvgl.h>
#include "perf_test.h"
#include "perf_prof.h"
#include "ui/ui_manager.h"
#include "ui/widgets/spot_table.h"

extern uint32_t g_perfFlushes, g_perfFlushUs, g_perfPixels, g_perfFrames;
static uint32_t perfHandlerCalls, perfHandlerMaxUs, perfHandlerTotUs, pt0;
static uint32_t perfStart, perfLast;
static lv_timer_t* perfTimer;
static int perfPhase;
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

void perfTestInit() { uiRequest(ScreenId::Settings); }

void perfBeforeHandler() { pt0 = micros(); }

void perfAfterHandler() {
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
}
#endif
