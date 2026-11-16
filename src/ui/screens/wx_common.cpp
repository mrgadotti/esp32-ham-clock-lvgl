#include "wx_common.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "i18n.h"
#include "../../app/config_store.h"
#include "../../net/time_service.h"
#include "../fonts/fonts.h"
#include "../icons/weather_icons.h"
#include "../theme.h"

lv_obj_t* wxLabel(lv_obj_t* p, const lv_font_t* f, lv_color_t col, int x, int y, int w, lv_text_align_t al) {
  lv_obj_t* l = lv_label_create(p);
  lv_obj_set_style_text_font(l, f, 0);
  lv_obj_set_style_text_color(l, col, 0);
  lv_obj_set_style_text_align(l, al, 0);
  if (w) lv_obj_set_width(l, w);
  lv_obj_set_pos(l, x, y);
  lv_label_set_text(l, "");
  return l;
}

lv_obj_t* wxIconCreate(lv_obj_t* p, int x, int y) {
  lv_obj_t* s = lv_obj_create(p);
  lv_obj_remove_style_all(s);
  lv_obj_set_size(s, 50, 50);
  lv_obj_set_pos(s, x, y);
  lv_obj_t* fb = lv_obj_create(s);  // child 0: fallback circle + "?"
  lv_obj_remove_style_all(fb);
  lv_obj_set_size(fb, 40, 40);
  lv_obj_center(fb);
  lv_obj_set_style_radius(fb, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(fb, 2, 0);
  lv_obj_set_style_border_color(fb, lv_color_hex(0x808080), 0);
  lv_obj_t* q = lv_label_create(fb);
  lv_obj_set_style_text_font(q, &font_24, 0);
  lv_obj_set_style_text_color(q, lv_color_hex(0x808080), 0);
  lv_label_set_text(q, "?");
  lv_obj_center(q);
  lv_obj_t* im = lv_image_create(s);  // child 1
  lv_obj_add_flag(im, LV_OBJ_FLAG_HIDDEN);
  return s;
}

void wxIconSet(lv_obj_t* s, int id, bool night) {
  const lv_image_dsc_t* d = weatherIcon(id, night);
  lv_obj_t* im = lv_obj_get_child(s, 1);
  if (d) lv_image_set_src(im, d);
  lv_obj_set_flag(im, LV_OBJ_FLAG_HIDDEN, d == nullptr);
  lv_obj_set_flag(lv_obj_get_child(s, 0), LV_OBJ_FLAG_HIDDEN, d != nullptr);
}

lv_color_t wxWindColor(float ms) {
  if (ms < 5) return COL_MODE;
  if (ms < 10) return COL_FREQ;
  if (ms < 15) return lv_color_hex(0xFF7F00);
  return COL_ERROR;
}

void wxLocalClock(char* out, size_t n, struct tm* tmOut) {
  if (!timeValid()) {
    snprintf(out, n, "--:--");
    if (tmOut) memset(tmOut, 0, sizeof *tmOut);
    return;
  }
  time_t t = time(nullptr) + (time_t)(cfg().timezone * 3600.0f);
  struct tm tm;
  gmtime_r(&t, &tm);
  snprintf(out, n, "%02d:%02d", tm.tm_hour, tm.tm_min);
  if (tmOut) *tmOut = tm;
}

void wxFooter(char* out, size_t n, const WeatherData& w, bool haveKey) {
  if (!haveKey) snprintf(out, n, "OpenWeather API key not set");
  else if (!w.valid) snprintf(out, n, "%s", w.lastError[0] ? w.lastError : tr(Tr::WaitingForData));
  else {
    unsigned mins = w.fetchedAt ? (unsigned)((millis() - w.fetchedAt) / 60000UL) : 0;
    snprintf(out, n, "%s%s  (%u %s)", tr(Tr::WeatherForLocation), w.cityName, mins, tr(Tr::Min));
  }
}
