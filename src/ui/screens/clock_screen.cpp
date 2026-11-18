#include "clock_screen.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "hold_detector.h"
#include "i18n.h"
#include "../../app/events.h"
#include "../../app/config_store.h"
#include "../../app/data_store.h"
#include "../../net/time_service.h"
#include "../fonts/fonts.h"
#include "../theme.h"
#include "../widgets/header.h"
#include "../widgets/ui_kit.h"
#include "../ui_manager.h"

namespace {
constexpr int DATE_W = 296;

class ClockScreen : public Screen {
 public:
  ScreenId id() const override { return ScreenId::HamClock; }

  void create(lv_obj_t* p) override {
    lv_obj_t* frame = lv_obj_create(p);
    lv_obj_remove_style_all(frame);
    lv_obj_set_size(frame, 300, 172);  // bottom edge at y=182: >= 6 px below the date text (ends ~y=164), clear of the IP line (y=200)
    lv_obj_set_pos(frame, 10, 10);
    lv_obj_set_style_radius(frame, 8, 0);
    lv_obj_set_style_border_width(frame, 2, 0);
    lv_obj_set_style_border_color(frame, lv_color_hex(0x404040), 0);
    lv_obj_remove_flag(frame, LV_OBJ_FLAG_SCROLLABLE);

    char up[16];
    snprintf(up, sizeof up, "%s", cfg().user_callsign[0] ? cfg().user_callsign : "SWL");
    for (char* c = up; *c; c++) if (*c >= 'a' && *c <= 'z') *c -= 32;
    uiLabel(p, &font_24, COL_TEXT, up, 0, 18, SCREEN_W, LV_TEXT_ALIGN_CENTER);
    uiLabel(p, &font_14, COL_ACCENT, tr(Tr::OperatorStation), 0, 48, SCREEN_W, LV_TEXT_ALIGN_CENTER);
    // N-05: the per-second label is only as wide as its text (214 px + slack) so each tick invalidates less
    time_ = uiLabel(p, &font_clock_48, COL_TEXT, "", 48, 76, 224, LV_TEXT_ALIGN_CENTER);
    lv_label_set_long_mode(time_, LV_LABEL_LONG_CLIP);
    mode_ = uiLabel(p, &font_14, COL_FREQ, "", 0, 124, SCREEN_W, LV_TEXT_ALIGN_CENTER);
    lv_obj_add_flag(mode_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(mode_, 20);
    lv_obj_add_event_cb(mode_, onMode, LV_EVENT_CLICKED, nullptr);
    date_ = uiLabel(p, &font_18, COL_TEXT, "", (SCREEN_W - DATE_W) / 2, 142, DATE_W, LV_TEXT_ALIGN_CENTER);
    lv_label_set_long_mode(date_, LV_LABEL_LONG_CLIP);
    ip_ = uiLabel(p, &font_18, COL_TEXT, "", 0, 200, SCREEN_W, LV_TEXT_ALIGN_CENTER);

    dot_ = lv_obj_create(p);
    lv_obj_remove_style_all(dot_);
    lv_obj_set_style_radius(dot_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(dot_, LV_OPA_COVER, 0);
    lv_obj_set_size(dot_, 10, 10);
    lv_obj_set_pos(dot_, 15, 207);
    // Gear (Settings): 36x36 touch target around the FontAwesome gear carried by font_24.
    lv_obj_t* gear = lv_obj_create(p);
    lv_obj_remove_style_all(gear);
    lv_obj_set_size(gear, 36, 36);
    lv_obj_set_pos(gear, 280, 200);
    lv_obj_add_flag(gear, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(gear, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(gear, [](lv_event_t*) { uiRequest(ScreenId::Settings); }, LV_EVENT_CLICKED, nullptr);  // deferred switch
    lv_obj_t* g = lv_label_create(gear);
    lv_obj_set_style_text_font(g, &font_24, 0);
    lv_obj_set_style_text_color(g, lv_color_hex(0xA0A0A0), 0);
    lv_label_set_text(g, LV_SYMBOL_SETTINGS);
    lv_obj_center(g);
    // Long press anywhere on the screen (gear/toggle are clickable and keep their own events) opens touch calibration.
    lv_obj_add_event_cb(p, onTouch, LV_EVENT_ALL, this);
    bar_ = lv_obj_create(p);
    lv_obj_remove_style_all(bar_);
    lv_obj_set_style_bg_opa(bar_, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(bar_, COL_ACCENT, 0);
    lv_obj_set_pos(bar_, 0, 238);
    lv_obj_set_height(bar_, 2);
    lv_obj_add_flag(bar_, LV_OBJ_FLAG_HIDDEN);
    lastSec_ = -1;
    refresh();
  }

  void tick(uint32_t) override {
    refresh();
    if (!hold_.active()) {
      lv_obj_add_flag(bar_, LV_OBJ_FLAG_HIDDEN);
      return;
    }
    uint32_t now = lv_tick_get();
    if (hold_.update(now)) {
      lv_obj_add_flag(bar_, LV_OBJ_FLAG_HIDDEN);
      UiEvent e;
      e.kind = EventKind::StartTouchCal;  // deferred: handled by UiManager outside any widget callback
      postUiEvent(e);
      return;
    }
    uint32_t el = hold_.elapsed(now);
    if (el < kBarStartMs) return;
    lv_obj_set_width(bar_, (int)((uint64_t)SCREEN_W * (el - kBarStartMs) / (HoldDetector::kHoldMs - kBarStartMs)));
    lv_obj_remove_flag(bar_, LV_OBJ_FLAG_HIDDEN);
  }

 private:
  static constexpr uint32_t kBarStartMs = 5000;
  HoldDetector hold_;
  lv_obj_t* bar_;
  lv_obj_t *time_, *mode_, *date_, *ip_, *dot_;
  int lastSec_, lastDay_ = -1;
  char lastIp_[16] = "";
  bool lastOk_ = false;

  static void onTouch(lv_event_t* e) {
    auto* self = (ClockScreen*)lv_event_get_user_data(e);
    lv_event_code_t c = lv_event_get_code(e);
    if (c == LV_EVENT_PRESSED || c == LV_EVENT_PRESSING) {
      lv_point_t pt;
      lv_indev_get_point(lv_indev_active(), &pt);
      if (c == LV_EVENT_PRESSED) self->hold_.press(pt.x, pt.y, lv_tick_get());
      else self->hold_.move(pt.x, pt.y);
    } else if (c == LV_EVENT_RELEASED || c == LV_EVENT_PRESS_LOST) {
      self->hold_.release();
    }
  }

  static void onMode(lv_event_t*) {
    cfgUpdate([](Config& c) { c.screen1_time = c.screen1_time ? 0 : 1; });
  }

  // "Weekday, 30 Month 2026"; when it is wider than the 296 px label (Wednesday, 30 September 2026 is 300 px) the
  // 3-letter weekday is used (tools/check_text_fit.py models the same rule).
  void setDate(const struct tm& tm) {
    char d[40];
    const char* mon = tr((Tr)((int)Tr::MonthJanuary + tm.tm_mon));
    snprintf(d, sizeof d, "%s, %d %s %d", tr((Tr)((int)Tr::WeekdaySunday + tm.tm_wday)), tm.tm_mday, mon, tm.tm_year + 1900);
    lv_point_t sz;
    lv_text_get_size(&sz, d, &font_18, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    if (sz.x > DATE_W) snprintf(d, sizeof d, "%s, %d %s %d", tr((Tr)((int)Tr::DaySun + tm.tm_wday)), tm.tm_mday, mon, tm.tm_year + 1900);
    lv_label_set_text(date_, d);
  }

  void refresh() {
    bool local = cfg().screen1_time != 0;
    if (timeValid()) {
      time_t now = time(nullptr);
      struct tm tm;
      now += uiTimeOffsetSec();
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
    g_store.net.read([&](const NetStatus& s) { n = s; });
    bool ok = n.sta || n.ap;
    if (ok != lastOk_ || strcmp(n.ip, lastIp_) != 0 || first_) {
      first_ = false;
      lastOk_ = ok;
      snprintf(lastIp_, sizeof lastIp_, "%s", n.ip);
      lv_label_set_text(ip_, ok ? n.ip : tr(Tr::NoConnection));
      lv_obj_set_style_text_color(ip_, ok ? COL_GREY : COL_ERROR, 0);
      lv_obj_set_style_bg_color(dot_, ok ? COL_MODE : COL_ERROR, 0);
    }
  }
  bool lastLocal_ = false, first_ = true;
};
}  // namespace

Screen* createClockScreen() { return new ClockScreen(); }
