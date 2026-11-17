#include "boot_screen.h"

#include "i18n.h"
#include "../../app/config_store.h"
#include "../../net/time_service.h"
#include "../fonts/fonts.h"
#include "../theme.h"

static lv_obj_t* log;

lv_obj_t* bootScreenCreate() {
  lv_obj_t* s = lv_obj_create(nullptr);
  lv_obj_t* t = lv_label_create(s);
  lv_obj_set_style_text_font(t, &font_24, 0);
  lv_obj_set_style_text_color(t, COL_ACCENT, 0);
  lv_label_set_text(t, "ESP32-HAM-CLOCK");
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 40);
  lv_obj_t* c = lv_label_create(s);
  lv_obj_set_style_text_font(c, &font_24, 0);
  lv_label_set_text(c, cfg().user_callsign);
  lv_obj_align(c, LV_ALIGN_TOP_MID, 0, 80);
  log = lv_label_create(s);
  lv_obj_set_style_text_font(log, &font_14, 0);
  lv_obj_add_event_cb(log, [](lv_event_t*) { log = nullptr; }, LV_EVENT_DELETE, nullptr);
  lv_obj_align(log, LV_ALIGN_BOTTOM_LEFT, 10, -10);
  bootScreenUpdate(false);
  return s;
}

void bootScreenUpdate(bool wifiUp) {
  if (!log) return;
  lv_label_set_text_fmt(log, "%s\nWiFi: %s\nNTP: %s", tr(Tr::Booting), wifiUp ? "OK" : "...",
                        timeValid() ? "OK" : "...");
  lv_obj_align(log, LV_ALIGN_BOTTOM_LEFT, 10, -10);
}
