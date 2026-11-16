#include "settings_screen.h"

#include <Arduino.h>
#include <stdio.h>

#include "i18n.h"
#include "../../app/config_store.h"
#include "../../app/data_store.h"
#include "../../hal/display.h"
#include "../../hal/touch_cal_ui.h"
#include "../fonts/fonts.h"
#include "../theme.h"
#include "../widgets/header.h"

namespace {
// cfgUpdate takes a plain function pointer: values travel through these (UI task only).
int32_t pend;
int32_t lastNdim = 50;  // RAM only: restored when the night-dimming switch is turned back on

const ScreenId kScreens[] = {ScreenId::HamClock, ScreenId::DxCluster, ScreenId::SunSpots, ScreenId::BandInfo,
                             ScreenId::Weather, ScreenId::AprsList, ScreenId::Pota, ScreenId::HamAlert,
                             ScreenId::AprsRadar, ScreenId::Matrix, ScreenId::Forecast, ScreenId::Location};
const Tr kScreenNames[] = {Tr::Clock, Tr::DxCluster, Tr::Propagation, Tr::HfBands, Tr::Weather, Tr::AprsIs,
                           Tr::Pota, Tr::HamAlert, Tr::AprsRadar, Tr::Matrix, Tr::Forecast, Tr::Location};
const int32_t kSwSec[] = {10, 15, 30, 60, 120, 300};

lv_obj_t* row(lv_obj_t* list, const char* text, lv_obj_t** labelOut = nullptr) {
  lv_obj_t* r = lv_obj_create(list);
  lv_obj_remove_style_all(r);
  lv_obj_set_size(r, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(r, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_t* l = lv_label_create(r);
  lv_obj_set_style_text_font(l, &font_14, 0);
  lv_label_set_text(l, text);
  if (labelOut) *labelOut = l;
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
  lv_slider_set_range(s, lo, hi);
  lv_slider_set_value(s, v, LV_ANIM_OFF);
  return s;
}

class SettingsScreen : public Screen {
 public:
  ScreenId id() const override { return ScreenId::Settings; }

  void create(lv_obj_t* p) override {
    headerCreate(p, Tr::Settings);
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
      pend = lv_slider_get_value((lv_obj_t*)lv_event_get_target(e));
      cfgUpdate([](Config& c) { c.tft_backlight = pend; });
    }, LV_EVENT_RELEASED, nullptr);

    if (c.tft_ndim > 0) lastNdim = c.tft_ndim;
    lv_obj_t* r = row(list, tr(Tr::NightDim));
    nd_ = lv_switch_create(r);
    if (c.tft_ndim > 0) lv_obj_add_state(nd_, LV_STATE_CHECKED);
    ndSlider_ = slider(list, 0, 90, c.tft_ndim);
    lv_obj_add_event_cb(nd_, [](lv_event_t* e) {
      bool on = lv_obj_has_state((lv_obj_t*)lv_event_get_target(e), LV_STATE_CHECKED);
      pend = on ? lastNdim : 0;
      cfgUpdate([](Config& c) { c.tft_ndim = pend; });
      lv_slider_set_value((lv_obj_t*)lv_event_get_user_data(e), pend, LV_ANIM_OFF);
    }, LV_EVENT_VALUE_CHANGED, ndSlider_);
    lv_obj_add_event_cb(ndSlider_, [](lv_event_t* e) {
      pend = lv_slider_get_value((lv_obj_t*)lv_event_get_target(e));
      if (pend > 0) lastNdim = pend;
      cfgUpdate([](Config& c) { c.tft_ndim = pend; });
    }, LV_EVENT_RELEASED, nullptr);

    lv_obj_t* b = lv_button_create(list);
    lv_obj_t* bl = lv_label_create(b);
    lv_obj_set_style_text_font(bl, &font_14, 0);
    lv_label_set_text(bl, tr(Tr::CalibrateTouch));
    lv_obj_add_event_cb(b, [](lv_event_t*) { touchCalRun(nullptr); }, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* sw = lv_switch_create(row(list, tr(Tr::InvertColors)));
    if (c.tft_inv) lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, [](lv_event_t* e) {
      pend = lv_obj_has_state((lv_obj_t*)lv_event_get_target(e), LV_STATE_CHECKED);
      displaySetInvert(pend);
      cfgUpdate([](Config& c) { c.tft_inv = pend; });
    }, LV_EVENT_VALUE_CHANGED, nullptr);

    // rotation applies via main.cpp applyCfgToHal on ConfigChanged
    dd_[0] = dropdown(list, tr(Tr::Rotation), "0\xC2\xB0\n180\xC2\xB0", c.tft_rot == 3, [](lv_event_t* e) {
      pend = lv_dropdown_get_selected((lv_obj_t*)lv_event_get_target(e)) ? 3 : 1;
      cfgUpdate([](Config& c) { c.tft_rot = pend; });
    });
    dd_[1] = dropdown(list, tr(Tr::Language), "English\nPortugu\xC3\xAAs", c.tft_lang ? 1 : 0, [](lv_event_t* e) {
      pend = lv_dropdown_get_selected((lv_obj_t*)lv_event_get_target(e));
      cfgUpdate([](Config& c) { c.tft_lang = (uint8_t)pend; });
    });
    uint32_t sel = 0;
    if (c.tft_autosw)
      for (uint32_t i = 0; i < 6; i++) if (kSwSec[i] == c.tft_sw_sec) sel = i + 1;
    dd_[2] = dropdown(list, tr(Tr::AutoSwitch), "Off\n10 s\n15 s\n30 s\n60 s\n120 s\n300 s", sel, [](lv_event_t* e) {
      pend = lv_dropdown_get_selected((lv_obj_t*)lv_event_get_target(e));
      cfgUpdate([](Config& c) {
        c.tft_autosw = pend > 0;
        if (pend > 0) c.tft_sw_sec = kSwSec[pend - 1];
      });
    });

    lv_obj_t* h = lv_label_create(list);
    lv_obj_set_style_text_font(h, &font_14, 0);
    lv_obj_set_style_text_color(h, COL_ACCENT, 0);
    lv_label_set_text(h, tr(Tr::EnabledScreens));
    for (int i = 0; i < 12; i++) {
      lv_obj_t* cb = lv_checkbox_create(list);
      lv_obj_set_style_text_font(cb, &font_14, 0);
      lv_checkbox_set_text(cb, tr(kScreenNames[i]));
      if (c.scr_en & (1 << (int)kScreens[i])) lv_obj_add_state(cb, LV_STATE_CHECKED);
      lv_obj_add_event_cb(cb, [](lv_event_t* e) {
        lv_obj_t* t = (lv_obj_t*)lv_event_get_target(e);
        pend = (int32_t)(intptr_t)lv_event_get_user_data(e) | (lv_obj_has_state(t, LV_STATE_CHECKED) ? 0x40000000 : 0);
        cfgUpdate([](Config& c) {
          int32_t bit = 1 << (pend & 0xFF);
          c.scr_en = (pend & 0x40000000) ? (c.scr_en | bit) : (c.scr_en & ~bit);
        });
      }, LV_EVENT_VALUE_CHANGED, (void*)(intptr_t)(int)kScreens[i]);
    }

    for (lv_obj_t*& l : info_) {
      l = lv_label_create(list);
      lv_obj_set_style_text_font(l, &font_14, 0);
    }
    lv_label_set_text_fmt(info_[4], "%s: %s %s", tr(Tr::Firmware), __DATE__, __TIME__);
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
    lv_label_set_text_fmt(info_[0], "SSID: %s", n.ssid);
    lv_label_set_text_fmt(info_[1], "IP: %s", n.ip);
    lv_label_set_text_fmt(info_[2], "RSSI: %d dBm", (int)n.rssi);
    lv_label_set_text_fmt(info_[3], "%s: %u B", tr(Tr::FreeHeap), (unsigned)ESP.getFreeHeap());
  }

  lv_obj_t *nd_ = nullptr, *ndSlider_ = nullptr, *dd_[3] = {}, *info_[5] = {};
  uint32_t lastInfo_ = 0;
};
}  // namespace

Screen* createSettingsScreen() { return new SettingsScreen(); }
