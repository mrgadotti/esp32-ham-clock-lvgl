#include "boot_screen.h"

#include "../../app/boot_log.h"
#include "../../app/config_store.h"
#include "../fonts/fonts.h"
#include "../theme.h"

// Title (y 8..37) and callsign (y 40..69) leave 8 font_14 lines (18 px) from y=76 to 220.
static constexpr int kVisible = 8, kLineH = 18, kTop = 76;
static lv_obj_t* rows[kVisible];
static uint32_t shownVer;

lv_obj_t* bootScreenCreate() {
  lv_obj_t* s = lv_obj_create(nullptr);
  lv_obj_t* t = lv_label_create(s);
  lv_obj_set_style_text_font(t, &font_24, 0);
  lv_obj_set_style_text_color(t, COL_ACCENT, 0);
  lv_label_set_text(t, "ESP32-HAM-CLOCK");
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 8);
  lv_obj_t* c = lv_label_create(s);
  lv_obj_set_style_text_font(c, &font_24, 0);
  lv_label_set_text(c, cfg().user_callsign);
  lv_obj_align(c, LV_ALIGN_TOP_MID, 0, 40);
  for (int i = 0; i < kVisible; i++) {
    lv_obj_t* l = lv_label_create(s);
    lv_obj_set_style_text_font(l, &font_14, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(0x8a8a8a), 0);  // dim grey
    lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);              // one line: ellipsis instead of overflow
    lv_obj_set_size(l, 300, kLineH);
    lv_obj_set_pos(l, 10, kTop + i * kLineH);
    lv_obj_add_event_cb(l, [](lv_event_t* e) {
      lv_obj_t* o = (lv_obj_t*)lv_event_get_target(e);
      for (auto& r : rows) if (r == o) r = nullptr;
    }, LV_EVENT_DELETE, nullptr);
    rows[i] = l;
  }
  shownVer = 0;
  bootScreenUpdate();
  return s;
}

void bootScreenUpdate() {
  uint32_t v = bootLogVersion();
  if (!rows[0] || v == shownVer) return;
  shownVer = v;
  char lines[kVisible][40];
  size_t n = bootLogSnapshot(lines, kVisible);
  for (int i = 0; i < kVisible; i++) {  // newest at the bottom
    int src = (int)n - kVisible + i;
    if (rows[i]) lv_label_set_text(rows[i], src >= 0 ? lines[src] : "");
  }
}
