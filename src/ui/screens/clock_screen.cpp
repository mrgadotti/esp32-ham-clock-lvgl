#include "clock_screen.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "i18n.h"
#include "../../app/config_store.h"
#include "../../app/data_store.h"
#include "../../net/time_service.h"
#include "../fonts/fonts.h"
#include "../theme.h"

namespace {
class ClockScreen : public Screen {
 public:
  ScreenId id() const override { return ScreenId::HamClock; }

  void create(lv_obj_t* p) override {
    lv_obj_t* frame = lv_obj_create(p);
    lv_obj_remove_style_all(frame);
    lv_obj_set_size(frame, 300, 155);
    lv_obj_set_pos(frame, 10, 10);
    lv_obj_set_style_radius(frame, 8, 0);
    lv_obj_set_style_border_width(frame, 2, 0);
    lv_obj_set_style_border_color(frame, lv_color_hex(0x404040), 0);
    lv_obj_remove_flag(frame, LV_OBJ_FLAG_SCROLLABLE);

    call_ = lbl(p, &font_24, 18, COL_TEXT);
    char up[16];
    snprintf(up, sizeof up, "%s", cfg().user_callsign[0] ? cfg().user_callsign : "SWL");
    for (char* c = up; *c; c++) if (*c >= 'a' && *c <= 'z') *c -= 32;
    lv_label_set_text(call_, up);
    lv_obj_t* op = lbl(p, &font_14, 48, COL_ACCENT);
    lv_label_set_text(op, tr(Tr::OperatorStation));
    time_ = lbl(p, &font_clock_48, 68, COL_TEXT);
    mode_ = lbl(p, &font_14, 124, COL_FREQ);
    lv_obj_add_flag(mode_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(mode_, 20);
    lv_obj_add_event_cb(mode_, onMode, LV_EVENT_CLICKED, nullptr);
    date_ = lbl(p, &font_18, 142, COL_TEXT);
    ip_ = lbl(p, &font_18, 200, COL_TEXT);

    dot_ = lv_obj_create(p);
    lv_obj_remove_style_all(dot_);
    lv_obj_set_style_radius(dot_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(dot_, LV_OPA_COVER, 0);
    lv_obj_set_size(dot_, 10, 10);
    lv_obj_set_pos(dot_, 15, 207);
    gps_ = lv_label_create(p);
    lv_obj_set_style_text_font(gps_, &font_14, 0);
    lv_obj_set_style_text_color(gps_, COL_MODE, 0);
    lv_label_set_text(gps_, tr(Tr::Gps));
    lv_obj_set_pos(gps_, 30, 205);
    lv_obj_add_flag(gps_, LV_OBJ_FLAG_HIDDEN);
    lastSec_ = -1;
    refresh();
  }

  void tick(uint32_t) override { refresh(); }

 private:
  lv_obj_t *call_, *time_, *mode_, *date_, *ip_, *dot_, *gps_;
  int lastSec_;
  char lastIp_[16] = "";
  bool lastOk_ = false, lastGps_ = false;

  static lv_obj_t* lbl(lv_obj_t* p, const lv_font_t* f, int y, lv_color_t col) {
    lv_obj_t* l = lv_label_create(p);
    lv_obj_set_style_text_font(l, f, 0);
    lv_obj_set_style_text_color(l, col, 0);
    lv_obj_set_width(l, 320);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(l, 0, y);
    return l;
  }

  static void onMode(lv_event_t*) {
    cfgUpdate([](Config& c) { c.screen1_time = c.screen1_time ? 0 : 1; });
  }

  void refresh() {
    bool local = cfg().screen1_time != 0;
    if (timeValid()) {
      time_t now = time(nullptr);
      struct tm tm;
      if (local) now += (time_t)(cfg().timezone * 3600.0f);  // UI snapshot, same math as localTimeOf
      gmtime_r(&now, &tm);
      if (tm.tm_sec != lastSec_ || lastLocal_ != local) {
        lastSec_ = tm.tm_sec;
        lastLocal_ = local;
        lv_label_set_text_fmt(time_, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
        lv_label_set_text(mode_, tr(local ? Tr::LocalTime : Tr::Utc));
        lv_label_set_text_fmt(date_, "%s, %d %s %d", tr((Tr)((int)Tr::WeekdaySunday + tm.tm_wday)), tm.tm_mday,
                              tr((Tr)((int)Tr::MonthJanuary + tm.tm_mon)), tm.tm_year + 1900);
      }
    } else if (lastSec_ != -2 || lastLocal_ != local) {
      lastSec_ = -2;
      lastLocal_ = local;
      lv_label_set_text(time_, "--:--:--");
      lv_label_set_text(mode_, tr(local ? Tr::LocalTime : Tr::Utc));
      lv_label_set_text(date_, "");
    }

    NetStatus n;
    GpsState g;
    g_store.net.read([&](const NetStatus& s) { n = s; });
    g_store.gps.read([&](const GpsState& s) { g = s; });
    bool ok = n.sta || n.ap;
    bool gpsFix = g.active && g.fix;
    if (ok != lastOk_ || strcmp(n.ip, lastIp_) != 0 || first_) {
      first_ = false;
      lastOk_ = ok;
      snprintf(lastIp_, sizeof lastIp_, "%s", n.ip);
      lv_label_set_text(ip_, ok ? n.ip : tr(Tr::NoConnection));
      lv_obj_set_style_text_color(ip_, ok ? lv_color_hex(0xC0C0C0) : COL_ERROR, 0);
      lv_obj_set_style_bg_color(dot_, ok ? COL_MODE : COL_ERROR, 0);
    }
    if (gpsFix != lastGps_) {
      lastGps_ = gpsFix;
      lv_obj_set_flag(gps_, LV_OBJ_FLAG_HIDDEN, !gpsFix);
    }
  }
  bool lastLocal_ = false, first_ = true;
};
}  // namespace

Screen* createClockScreen() { return new ClockScreen(); }
