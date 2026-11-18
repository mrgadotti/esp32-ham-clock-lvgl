#include "header.h"
#include "../theme.h"
#include "../fonts/fonts.h"
#include "../../app/config_store.h"
#include "../../net/time_service.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

int32_t uiTimeOffsetSec() { return cfg().screen1_time ? (int32_t)(cfg().timezone * 3600.0f) : 0; }

void headerTickUtc(lv_obj_t* header) {
  char s[12] = "--:--";
  if (timeValid()) {
    time_t t = time(nullptr) + uiTimeOffsetSec();
    struct tm tm;
    gmtime_r(&t, &tm);
    snprintf(s, sizeof s, "%02d:%02d %c", tm.tm_hour, tm.tm_min, cfg().screen1_time ? 'L' : 'Z');
  }
  lv_obj_t* c = static_cast<lv_obj_t*>(lv_obj_get_user_data(header));
  if (c && lv_label_get_text(c) && strcmp(lv_label_get_text(c), s) != 0) headerSetClock(header, s);
}

lv_obj_t* headerCreate(lv_obj_t* parent, Tr title) {
  lv_obj_t* bar = lv_obj_create(parent);
  lv_obj_remove_style_all(bar);
  lv_obj_set_size(bar, LV_PCT(100), HEADER_H);
  lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_color(bar, COL_ACCENT, 0);
  lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(bar, COL_BG, 0);
  lv_obj_set_style_text_font(bar, &font_18, 0);
  lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* t = lv_label_create(bar);
  lv_label_set_text(t, tr(title));
  lv_obj_align(t, LV_ALIGN_LEFT_MID, 6, 0);

  lv_obj_t* c = lv_label_create(bar);
  lv_label_set_text(c, "");
  lv_obj_align(c, LV_ALIGN_RIGHT_MID, -6, 0);
  lv_obj_set_user_data(bar, c);  // clock label
  return bar;
}

void headerSetClock(lv_obj_t* header, const char* text) {
  lv_obj_t* c = static_cast<lv_obj_t*>(lv_obj_get_user_data(header));
  if (!c) return;
  lv_label_set_text(c, text);
  lv_obj_align(c, LV_ALIGN_RIGHT_MID, -6, 0);
}

lv_obj_t* headerTitle(lv_obj_t* hdr) { return lv_obj_get_child(hdr, 0); }
