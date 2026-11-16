#include "location_screen.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "geo.h"
#include "i18n.h"
#include "../../app/config_store.h"
#include "../../app/data_store.h"
#include "../fonts/fonts.h"
#include "../theme.h"
#include "../widgets/header.h"

namespace {
double pendLat, pendLon;  // cfgUpdate takes a plain function pointer (UI task only)

class LocationScreen : public Screen {
 public:
  ScreenId id() const override { return ScreenId::Location; }

  void create(lv_obj_t* p) override {
    hdr_ = headerCreate(p, Tr::Location);
    info_ = lv_label_create(p);
    lv_obj_set_style_text_font(info_, &font_18, 0);
    lv_obj_set_style_text_line_space(info_, 6, 0);
    lv_obj_align(info_, LV_ALIGN_TOP_LEFT, 8, HEADER_H + 8);
    lv_obj_t* b = lv_button_create(p);
    lv_obj_set_size(b, 110, 40);
    lv_obj_align(b, LV_ALIGN_BOTTOM_RIGHT, -8, -8);
    lv_obj_add_event_cb(b, onSave, LV_EVENT_CLICKED, this);
    lv_obj_t* l = lv_label_create(b);
    lv_label_set_text(l, tr(Tr::Save));
    lv_obj_center(l);
    toast_ = lv_label_create(p);
    lv_obj_set_style_text_font(toast_, &font_18, 0);
    lv_obj_align(toast_, LV_ALIGN_BOTTOM_LEFT, 8, -8);  // below the 6 info lines (end at y~190)
    lv_label_set_text(toast_, "");
    refresh();
  }

  void tick(uint32_t now) override {
    headerTickUtc(hdr_);
    if (toastUntil_ && (int32_t)(now - toastUntil_) >= 0) {
      toastUntil_ = 0;
      lv_label_set_text(toast_, "");
    }
    if (g_store.gps.version() != ver_ || now - lastMs_ >= 1000) {  // 1 s: fix age flips without new data
      lastMs_ = now;
      refresh();
    }
  }

 private:
  lv_obj_t *hdr_, *info_, *toast_;
  uint32_t ver_ = 0, lastMs_ = 0, toastUntil_ = 0;
  GpsState g_{};

  bool fixNow() const { return g_.fix && millis() - g_.lastFixMs < 15000; }

  void refresh() {
    ver_ = g_store.gps.version();
    g_store.gps.read([&](const GpsState& v) { g_ = v; });
    bool fix = fixNow();
    char loc[7] = "------", buf[256];
    if (fix) latLonToLocator(g_.lat, g_.lon, loc);
    int n = snprintf(buf, sizeof buf, "%s %s  %s %s\n%s %u (G%u R%u E%u B%u)\n", tr(Tr::GpsLabel),
                     tr(g_.active ? Tr::GpsActive : Tr::GpsInactive), tr(Tr::Fix), tr(fix ? Tr::Yes : Tr::No), tr(Tr::Sats),
                     g_.sats, g_.satsGps, g_.satsGlo, g_.satsGal, g_.satsBds);
    if (fix)
      snprintf(buf + n, sizeof buf - n, "%s %.5f\n%s %.5f\n%s %.1f km/h\n%s %s", tr(Tr::Lat), g_.lat, tr(Tr::Lon), g_.lon,
               tr(Tr::Speed), g_.speedKmh, tr(Tr::Locator), loc);
    else
      snprintf(buf + n, sizeof buf - n, "%s --\n%s --\n%s --\n%s %s", tr(Tr::Lat), tr(Tr::Lon), tr(Tr::Speed),
               tr(Tr::Locator), loc);
    if (strcmp(lv_label_get_text(info_), buf)) lv_label_set_text(info_, buf);
  }

  static void onSave(lv_event_t* e) {
    auto* s = static_cast<LocationScreen*>(lv_event_get_user_data(e));
    s->refresh();
    bool fix = s->fixNow();
    if (fix) {
      pendLat = s->g_.lat;
      pendLon = s->g_.lon;
      cfgUpdate([](Config& c) {
        c.user_lat = pendLat;
        c.user_lon = pendLon;
        c.user_ll_ok = true;
      });
    }
    lv_label_set_text(s->toast_, tr(fix ? Tr::Saved : Tr::NoFix));
    s->toastUntil_ = millis() + 2000;
  }
};
}  // namespace

Screen* createLocationScreen() { return new LocationScreen(); }
