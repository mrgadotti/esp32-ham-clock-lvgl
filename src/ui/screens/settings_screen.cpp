#include "settings_screen.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "i18n.h"
#include "../../app/config_store.h"
#include "../../app/data_store.h"
#include "../../app/mem_log.h"
#include "../../hal/display.h"
#include "../../hal/touch_cal_ui.h"
#include "../fonts/fonts.h"
#include "../theme.h"
#include "../widgets/header.h"
#include "../widgets/ui_kit.h"
#include "../ui_manager.h"

namespace {
int32_t lastNdim = 50;  // RAM only: restored when the night-dimming switch is turned back on

const ScreenId kScreens[] = {ScreenId::HamClock, ScreenId::DxCluster, ScreenId::SunSpots, ScreenId::BandInfo,
                             ScreenId::Weather, ScreenId::AprsList, ScreenId::Pota, ScreenId::HamAlert,
                             ScreenId::AprsRadar, ScreenId::Forecast, ScreenId::Location};
const Tr kScreenNames[] = {Tr::Clock, Tr::DxCluster, Tr::Propagation, Tr::HfBands, Tr::Weather, Tr::AprsIs,
                           Tr::Pota, Tr::HamAlert, Tr::AprsRadar, Tr::Forecast, Tr::Location};
static_assert(sizeof kScreens / sizeof *kScreens == sizeof kScreenNames / sizeof *kScreenNames,
              "kScreens and kScreenNames must list the same screens");
const int32_t kSwSec[] = {10, 15, 30, 60, 120, 300};
const int32_t kAlertSec[] = {5, 10, 15, 30, 45, 60};  // alrt_scr_s presets (config range 1..60)
const int32_t kAprsKm[] = {5, 10, 15, 20, 30, 50, 100, 150};  // aprs_radius presets (config range 1..150)

lv_obj_t* row(lv_obj_t* list, const char* text) {
  lv_obj_t* r = lv_obj_create(list);
  lv_obj_remove_style_all(r);
  lv_obj_set_size(r, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(r, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  if (!text) return r;
  uiLabel(r, &font_14, COL_TEXT, text);
  return r;
}

lv_obj_t* dropdown(lv_obj_t* list, const char* title, const char* opts, uint32_t sel, lv_event_cb_t cb) {
  lv_obj_t* d = lv_dropdown_create(row(list, title));
  lv_obj_set_style_text_font(d, &font_14, 0);
  lv_obj_set_style_text_font(lv_dropdown_get_list(d), &font_14, 0);
  lv_obj_set_width(d, 120);
  lv_dropdown_set_options(d, opts);
  lv_dropdown_set_selected(d, sel);
  lv_obj_add_event_cb(d, cb, LV_EVENT_VALUE_CHANGED, nullptr);
  return d;
}

lv_obj_t* slider(lv_obj_t* parent, int32_t lo, int32_t hi, int32_t v) {
  lv_obj_t* s = lv_slider_create(parent);
  lv_obj_set_width(s, 120);
  // The knob is drawn outside the bar (radius + knob pad) and a parent clips children to its own area
  // (lv_obj_redraw), so reserve room around the bar inside the row and let the row draw the overhang.
  lv_obj_set_style_margin_hor(s, 16, 0);
  lv_obj_set_style_margin_ver(s, 8, 0);
  lv_obj_add_flag(parent, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  lv_slider_set_range(s, lo, hi);
  lv_slider_set_value(s, v, LV_ANIM_OFF);
  return s;
}

void button(lv_obj_t* list, Tr t, lv_event_cb_t cb) {
  lv_obj_t* b = lv_button_create(list);
  lv_obj_set_size(b, lv_pct(100), 32);  // identical full-width buttons
  lv_obj_t* l = lv_label_create(b);
  lv_obj_set_style_text_font(l, &font_14, 0);
  lv_label_set_text(l, tr(t));
  lv_obj_center(l);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, nullptr);
}

class SettingsScreen : public Screen {
 public:
  ScreenId id() const override { return ScreenId::Settings; }

  void create(lv_obj_t* p) override {
    lv_obj_t* hdr = headerCreate(p, Tr::Settings);
    lv_obj_align(lv_obj_get_child(hdr, 0), LV_ALIGN_LEFT_MID, 80, 0);  // title right of the Back tile
    uiTilePaint(uiTile(hdr, tr(Tr::Back), 4, 1, 70, 24, [](lv_event_t*) { uiRequest(ScreenId::HamClock); }, 0), false);
    lv_obj_t* list = lv_obj_create(p);
    lv_obj_remove_style_all(list);
    lv_obj_set_pos(list, 0, HEADER_H);
    lv_obj_set_size(list, 320, 240 - HEADER_H);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(list, 6, 0);
    lv_obj_set_style_pad_row(list, 8, 0);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLLABLE);
    const Config& c = cfg();

    lv_obj_t* s = slider(row(list, tr(Tr::Brightness)), 5, 100, c.tft_backlight);
    lv_obj_add_event_cb(s, [](lv_event_t* e) { displaySetBacklight(lv_slider_get_value((lv_obj_t*)lv_event_get_target(e))); },
                        LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(s, [](lv_event_t* e) {
      int32_t v = lv_slider_get_value((lv_obj_t*)lv_event_get_target(e));
      cfgUpdate([v](Config& c) { c.tft_backlight = v; });
    }, LV_EVENT_RELEASED, nullptr);

    if (c.tft_ndim > 0) lastNdim = c.tft_ndim;
    lv_obj_t* r = row(list, tr(Tr::NightDim));
    nd_ = lv_switch_create(r);
    if (c.tft_ndim > 0) lv_obj_add_state(nd_, LV_STATE_CHECKED);
    ndSlider_ = slider(row(list, nullptr), 0, 90, c.tft_ndim);
    lv_obj_add_event_cb(nd_, [](lv_event_t* e) {
      bool on = lv_obj_has_state((lv_obj_t*)lv_event_get_target(e), LV_STATE_CHECKED);
      int32_t v = on ? lastNdim : 0;
      cfgUpdate([v](Config& c) { c.tft_ndim = v; });
      lv_slider_set_value((lv_obj_t*)lv_event_get_user_data(e), v, LV_ANIM_OFF);
    }, LV_EVENT_VALUE_CHANGED, ndSlider_);
    lv_obj_add_event_cb(ndSlider_, [](lv_event_t* e) {
      int32_t v = lv_slider_get_value((lv_obj_t*)lv_event_get_target(e));
      if (v > 0) lastNdim = v;
      lv_obj_set_state((lv_obj_t*)lv_event_get_user_data(e), LV_STATE_CHECKED, v > 0);
      cfgUpdate([v](Config& c) { c.tft_ndim = v; });
    }, LV_EVENT_RELEASED, nd_);

    button(list, Tr::CalibrateTouch, [](lv_event_t*) { touchCalRun(nullptr); });

    lv_obj_t* sw = lv_switch_create(row(list, tr(Tr::InvertColors)));
    if (c.tft_inv) lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, [](lv_event_t* e) {
      bool on = lv_obj_has_state((lv_obj_t*)lv_event_get_target(e), LV_STATE_CHECKED);
      displaySetInvert(on);
      cfgUpdate([on](Config& c) { c.tft_inv = on; });
    }, LV_EVENT_VALUE_CHANGED, nullptr);

    // rotation applies via main.cpp applyCfgToHal on ConfigChanged
    dd_[0] = dropdown(list, tr(Tr::Rotation), "0\xC2\xB0\n180\xC2\xB0", c.tft_rot == 3, [](lv_event_t* e) {
      int32_t rot = lv_dropdown_get_selected((lv_obj_t*)lv_event_get_target(e)) ? 3 : 1;
      cfgUpdate([rot](Config& c) { c.tft_rot = rot; });
    });
    dd_[1] = dropdown(list, tr(Tr::Language), "English\nPortugu\xC3\xAAs", c.tft_lang ? 1 : 0, [](lv_event_t* e) {
      uint8_t lang = (uint8_t)lv_dropdown_get_selected((lv_obj_t*)lv_event_get_target(e));
      cfgUpdate([lang](Config& c) { c.tft_lang = lang; });
    });
    uint32_t sel = 0;
    if (c.tft_autosw) {
      int32_t best = 1 << 30;
      for (uint32_t i = 0; i < 6; i++) {
        int32_t d = abs(kSwSec[i] - c.tft_sw_sec);
        if (d < best) { best = d; sel = i + 1; }
      }
    }
    char swOpts[48];
    snprintf(swOpts, sizeof swOpts, "%s\n10 s\n15 s\n30 s\n60 s\n120 s\n300 s", tr(Tr::Off));
    dd_[2] = dropdown(list, tr(Tr::AutoSwitch), swOpts, sel, [](lv_event_t* e) {
      uint32_t sel = lv_dropdown_get_selected((lv_obj_t*)lv_event_get_target(e));
      cfgUpdate([sel](Config& c) {
        c.tft_autosw = sel > 0;
        if (sel > 0) c.tft_sw_sec = kSwSec[sel - 1];
      });
    });

    uint32_t asel = 0;
    for (uint32_t i = 1; i < sizeof kAlertSec / sizeof *kAlertSec; i++)
      if (abs(kAlertSec[i] - c.alrt_scr_s) < abs(kAlertSec[asel] - c.alrt_scr_s)) asel = i;
    dd_[3] = dropdown(list, tr(Tr::AlertTime), "5 s\n10 s\n15 s\n30 s\n45 s\n60 s", asel, [](lv_event_t* e) {
      int32_t v = kAlertSec[lv_dropdown_get_selected((lv_obj_t*)lv_event_get_target(e))];
      cfgUpdate([v](Config& c) { c.alrt_scr_s = v; });
    });

    uint32_t rsel = 0;
    for (uint32_t i = 1; i < sizeof kAprsKm / sizeof *kAprsKm; i++)
      if (abs(kAprsKm[i] - c.aprs_radius) < abs(kAprsKm[rsel] - c.aprs_radius)) rsel = i;
    dd_[4] = dropdown(list, tr(Tr::AprsRadius), "5 km\n10 km\n15 km\n20 km\n30 km\n50 km\n100 km\n150 km", rsel, [](lv_event_t* e) {
      int32_t v = kAprsKm[lv_dropdown_get_selected((lv_obj_t*)lv_event_get_target(e))];
      cfgUpdate([v](Config& c) { c.aprs_radius = v; });  // APRS-IS reconnects with the new filter (aprs_is.cpp sameSession)
    });

    uiLabel(list, &font_14, COL_ACCENT, tr(Tr::EnabledScreens));
    for (size_t i = 0; i < sizeof kScreens / sizeof *kScreens; i++) {
      lv_obj_t* cb = lv_checkbox_create(list);
      lv_obj_set_style_text_font(cb, &font_14, 0);
      lv_checkbox_set_text(cb, tr(kScreenNames[i]));
      if (kScreens[i] == ScreenId::HamClock) {  // always in the carousel: it hosts the gear that opens Settings
        lv_obj_add_state(cb, (lv_state_t)(LV_STATE_CHECKED | LV_STATE_DISABLED));
        continue;
      }
      if (c.scr_en & (1 << (int)kScreens[i])) lv_obj_add_state(cb, LV_STATE_CHECKED);
      lv_obj_add_event_cb(cb, [](lv_event_t* e) {
        lv_obj_t* t = (lv_obj_t*)lv_event_get_target(e);
        const int32_t bit = 1 << (int)(intptr_t)lv_event_get_user_data(e);
        const bool on = lv_obj_has_state(t, LV_STATE_CHECKED);
        cfgUpdate([bit, on](Config& c) { c.scr_en = on ? (c.scr_en | bit) : (c.scr_en & ~bit); });
      }, LV_EVENT_VALUE_CHANGED, (void*)(intptr_t)(int)kScreens[i]);
    }

    for (lv_obj_t*& l : info_) {
      l = lv_label_create(list);
      lv_obj_set_style_text_font(l, &font_14, 0);
      lv_obj_set_width(l, 300);
      lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
    }
    lv_label_set_text_fmt(info_[4], "%s: %s %s", tr(Tr::Firmware), __DATE__, __TIME__);
    for (char* b : cache_) b[0] = 0;
    lastInfo_ = 0;
    refreshInfo(0);
  }

  void tick(uint32_t now) override { refreshInfo(now); }

  bool blocksSwipe() const override {
    for (lv_obj_t* d : dd_) if (d && lv_dropdown_is_open(d)) return true;
    return false;
  }

 private:
  void refreshInfo(uint32_t now) {
    if (lastInfo_ && now - lastInfo_ < 1000) return;
    lastInfo_ = now ? now : 1;
    NetStatus n{};
    g_store.net.read([&](const NetStatus& v) { n = v; });
    char t[48];
    snprintf(t, sizeof t, "SSID: %s", n.ssid); setIf(0, t);
    snprintf(t, sizeof t, "IP: %s", n.ip); setIf(1, t);
    snprintf(t, sizeof t, "RSSI: %d dBm", (int)n.rssi); setIf(2, t);
    snprintf(t, sizeof t, "%s: %u B", tr(Tr::FreeHeap), (unsigned)ESP.getFreeHeap()); setIf(3, t);
  }
  void setIf(int i, const char* t) {
    if (strcmp(cache_[i], t) == 0) return;
    snprintf(cache_[i], sizeof cache_[i], "%s", t);
    lv_label_set_text(info_[i], t);
  }

  lv_obj_t *nd_ = nullptr, *ndSlider_ = nullptr, *dd_[5] = {}, *info_[5] = {};
  char cache_[4][48] = {};
  uint32_t lastInfo_ = 0;
};
}  // namespace

Screen* createSettingsScreen() { return new SettingsScreen(); }
