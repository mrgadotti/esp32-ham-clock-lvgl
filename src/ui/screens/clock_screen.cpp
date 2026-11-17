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
#include "../widgets/ui_kit.h"

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

    char up[16];
    snprintf(up, sizeof up, "%s", cfg().user_callsign[0] ? cfg().user_callsign : "SWL");
    for (char* c = up; *c; c++) if (*c >= 'a' && *c <= 'z') *c -= 32;
    uiLabel(p, &font_24, COL_TEXT, up, 0, 18, 320, LV_TEXT_ALIGN_CENTER);
    uiLabel(p, &font_14, COL_ACCENT, tr(Tr::OperatorStation), 0, 48, 320, LV_TEXT_ALIGN_CENTER);
    // N-05: the per-second label is only as wide as its text (214 px) so each tick invalidates less
    time_ = uiLabel(p, &font_clock_48, COL_TEXT, "", 52, 68, 216, LV_TEXT_ALIGN_CENTER);
    lv_label_set_long_mode(time_, LV_LABEL_LONG_CLIP);
    mode_ = uiLabel(p, &font_14, COL_FREQ, "", 0, 124, 320, LV_TEXT_ALIGN_CENTER);
    lv_obj_add_flag(mode_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(mode_, 20);
    lv_obj_add_event_cb(mode_, onMode, LV_EVENT_CLICKED, nullptr);
    date_ = uiLabel(p, &font_18, COL_TEXT, "", 12, 142, 296, LV_TEXT_ALIGN_CENTER);
    lv_label_set_long_mode(date_, LV_LABEL_LONG_CLIP);
    ip_ = uiLabel(p, &font_18, COL_TEXT, "", 0, 200, 320, LV_TEXT_ALIGN_CENTER);

    dot_ = lv_obj_create(p);
    lv_obj_remove_style_all(dot_);
    lv_obj_set_style_radius(dot_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(dot_, LV_OPA_COVER, 0);
    lv_obj_set_size(dot_, 10, 10);
    lv_obj_set_pos(dot_, 15, 207);
    gps_ = uiLabel(p, &font_14, COL_MODE, tr(Tr::Gps), 30, 205);
    lv_obj_add_flag(gps_, LV_OBJ_FLAG_HIDDEN);
    lastSec_ = -1;
    refresh();
  }

  void tick(uint32_t) override { refresh(); }

 private:
  lv_obj_t *time_, *mode_, *date_, *ip_, *dot_, *gps_;
  int lastSec_, lastDay_ = -1;
  char lastIp_[16] = "";
  bool lastOk_ = false, lastGps_ = false;

  static void onMode(lv_event_t*) {
    cfgUpdate([](Config& c) { c.screen1_time = c.screen1_time ? 0 : 1; });
  }

  // "Weekday, 30 Month 2026"; the longest English date (Wednesday, 30 September 2026) is 300 px and would overflow the
  // 296 px label, so a date over 27 bytes uses the 3-letter weekday (tools/check_text_fit.py models the same rule).
  void setDate(const struct tm& tm) {
    char d[40];
    const char* mon = tr((Tr)((int)Tr::MonthJanuary + tm.tm_mon));
    int n = snprintf(d, sizeof d, "%s, %d %s %d", tr((Tr)((int)Tr::WeekdaySunday + tm.tm_wday)), tm.tm_mday, mon,
                     tm.tm_year + 1900);
    if (n > 27) snprintf(d, sizeof d, "%s, %d %s %d", tr((Tr)((int)Tr::DaySun + tm.tm_wday)), tm.tm_mday, mon, tm.tm_year + 1900);
    lv_label_set_text(date_, d);
  }

  void refresh() {
    bool local = cfg().screen1_time != 0;
    if (timeValid()) {
      time_t now = time(nullptr);
      struct tm tm;
      if (local) now += (time_t)(cfg().timezone * 3600.0f);  // UI snapshot of the timezone
      gmtime_r(&now, &tm);
      bool localChg = lastLocal_ != local;
      if (tm.tm_sec != lastSec_ || localChg) {
        lastSec_ = tm.tm_sec;
        lastLocal_ = local;
        lv_label_set_text_fmt(time_, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
        int day = tm.tm_year * 366 + tm.tm_yday;
        if (day != lastDay_ || localChg) {  // mode and date only change at midnight or on toggle: not every second
          lastDay_ = day;
          lv_label_set_text(mode_, tr(local ? Tr::LocalTime : Tr::Utc));
          setDate(tm);
        }
      }
    } else if (lastSec_ != -2 || lastLocal_ != local) {
      lastSec_ = -2;
      lastLocal_ = local;
      lv_label_set_text(time_, "--:--:--");
      lv_label_set_text(mode_, tr(local ? Tr::LocalTime : Tr::Utc));
      lv_label_set_text(date_, "");
      lastDay_ = -1;
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
