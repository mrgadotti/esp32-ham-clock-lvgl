#include "aprs_list_screen.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "i18n.h"
#include "../../app/config_store.h"
#include "../../app/data_store.h"
#include "../../net/time_service.h"
#include "../fonts/fonts.h"
#include "../theme.h"
#include "../widgets/header.h"
#include "../widgets/spot_table.h"

namespace {
enum { SORT_TIME, SORT_CALL, SORT_DIST };

struct Menu {
  lv_obj_t* panel;
  lv_obj_t* tile[3];
  lv_obj_t* box[4];
  int sort;
  bool flag[4];  // beacon, alert, range, led
};
Menu* menu = nullptr;
Menu pend;  // cfgUpdate takes a plain function pointer

// Short names in the original's table order; sym = {table, code}.
bool symName(const char* s, const char*& out) {
  struct E { char t, c; Tr n; };
  static const E k[] = {{'/', '-', Tr::SymHouse}, {'/', '>', Tr::SymCar}, {'/', '#', Tr::SymDigi}, {'/', 'r', Tr::SymRepeater},
                        {'/', '[', Tr::SymHuman}, {'/', 'l', Tr::SymLaptop}, {'/', 'L', Tr::SymLogin}, {'/', '_', Tr::SymWx},
                        {'/', 's', Tr::SymShip}, {'/', 'a', Tr::SymAmbulance}, {'/', 'b', Tr::SymBike}, {'/', 'c', Tr::SymIcp},
                        {'/', 'd', Tr::SymFire}, {'/', 'f', Tr::SymFiretruck}, {'/', 'h', Tr::SymHospital}, {'/', 'y', Tr::SymYagi},
                        {'/', 'g', Tr::SymGlider}, {'/', 'k', Tr::SymTruck}, {'/', 'u', Tr::SymTruck}, {'/', 'v', Tr::SymVan},
                        {'/', 'V', Tr::SymHamClock}, {'/', 'p', Tr::SymRover}, {'/', 'o', Tr::SymEoc}, {'/', 't', Tr::SymTruckStop},
                        {'/', 'm', Tr::SymMicERepeater}, {'/', 'n', Tr::SymNode}, {'/', 'q', Tr::SymGrid},
                        {'L', 'a', Tr::SymLora}, {'L', '_', Tr::SymLora}, {'L', '#', Tr::SymLora}, {'L', '&', Tr::SymLora}};
  for (const E& e : k)
    if (e.t == s[0] && e.c == s[1]) { out = tr(e.n); return true; }
  return false;
}

void paintTile(lv_obj_t* t, bool on) {
  lv_obj_set_style_bg_color(t, on ? COL_ACCENT : lv_color_hex(0x808080), 0);
  lv_obj_set_style_text_color(t, on ? COL_BG : COL_TEXT, 0);
}

void closeMenu() {
  if (!menu) return;
  lv_obj_delete_async(menu->panel);  // may run inside a widget's own callback
  delete menu;
  menu = nullptr;
}

class AprsListScreen : public Screen {
 public:
  ~AprsListScreen() override {
    closeMenu();
    if (self_ == this) self_ = nullptr;
  }
  ScreenId id() const override { return ScreenId::AprsList; }
  bool blocksSwipe() const override { return menu != nullptr; }

  void create(lv_obj_t* p) override {
    self_ = this;
    sort_ = (int)cfg().aprs_sort;
    if (sort_ < 0 || sort_ > SORT_DIST) sort_ = SORT_TIME;
    cols_.n = 5;
    const int16_t w[] = {36, 90, 87, 48, 59};  // UTC CALL SYM KM FREQ, sized by tools/check_text_fit.py
    const lv_color_t c[] = {lv_color_hex(0xC0C0C0), COL_TEXT, lv_color_hex(0x00FFFF), COL_ACCENT, COL_FREQ};
    for (int i = 0; i < 5; i++) { cols_.width[i] = w[i]; cols_.color[i] = c[i]; }
    const char* cm = cfg().uart2_cmod;
    Tr title = !strcmp(cm, "aprs_tnc_kiss") ? Tr::AprsKiss : !strcmp(cm, "aprs_tnc") ? Tr::AprsTnc2 : Tr::AprsIs;
    hdr_ = headerCreate(p, title);
    lv_obj_t* tl = lv_obj_get_child(hdr_, 0);  // headerCreate adds the title first
    lv_obj_align(tl, LV_ALIGN_LEFT_MID, 46, 0);
    lv_obj_t* btn = lv_obj_create(hdr_);
    lv_obj_remove_style_all(btn);
    lv_obj_set_size(btn, 40, HEADER_H);
    lv_obj_set_pos(btn, 0, 0);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    for (int i = 0; i < 3; i++) {  // hamburger bars
      lv_obj_t* bar = lv_obj_create(btn);
      lv_obj_remove_style_all(bar);
      lv_obj_set_size(bar, 20, 3);
      lv_obj_set_pos(bar, 10, 6 + i * 6);
      lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
      lv_obj_set_style_bg_color(bar, COL_BG, 0);
    }
    lv_obj_add_event_cb(btn, onMenu, LV_EVENT_CLICKED, nullptr);

    const Tr hd[] = {Tr::Utc, Tr::ColCall, Tr::ColSymbol, Tr::ColKm, Tr::ColFreq};
    int x = 0;
    for (int i = 0; i < 5; i++) {
      lv_obj_t* l = lv_label_create(p);
      lv_obj_set_style_text_font(l, &font_14, 0);
      lv_obj_set_style_text_color(l, lv_color_hex(0xA0A0A0), 0);
      lv_label_set_text(l, tr(hd[i]));
      lv_obj_set_pos(l, x + 2, HEADER_H);
      x += cols_.width[i];
    }
    table_ = spotTableCreate(p, cols_);
    lv_obj_set_pos(table_, 0, HEADER_H + 18);
    lv_obj_set_height(table_, 240 - HEADER_H - 18);
    wait_ = lv_label_create(p);
    lv_obj_set_style_text_font(wait_, &font_18, 0);
    lv_obj_set_style_text_color(wait_, COL_ERROR, 0);
    lv_label_set_text(wait_, tr(Tr::WaitingForAprs));
    lv_obj_center(wait_);
    ver_ = ~g_store.aprs.version();
    refresh();
  }

  void tick(uint32_t) override {
    if (ver_ != g_store.aprs.version()) refresh();
    if (timeValid()) {
      time_t t = time(nullptr);
      struct tm tm;
      gmtime_r(&t, &tm);
      if (tm.tm_min != lastMin_) {
        lastMin_ = tm.tm_min;
        char b[12];
        snprintf(b, sizeof b, "%02d:%02d Z", tm.tm_hour, tm.tm_min);
        headerSetClock(hdr_, b);
      }
    }
  }

 private:
  static inline AprsListScreen* self_ = nullptr;
  SpotTableCols cols_{};
  lv_obj_t *hdr_, *table_, *wait_;
  int sort_;
  uint32_t ver_;
  int lastMin_ = -1;
  uint8_t idx_[30];
  size_t n_ = 0;
  size_t cachedRow_ = (size_t)-1;
  AprsStation cached_;

  static void refreshMenu() {
    for (int i = 0; i < 3; i++) paintTile(menu->tile[i], menu->sort == i);
  }

  static void onTile(lv_event_t* e) {
    if (!menu) return;
    menu->sort = (int)(intptr_t)lv_event_get_user_data(e);
    refreshMenu();
  }

  static void onBox(lv_event_t* e) {
    if (!menu) return;
    int i = (int)(intptr_t)lv_event_get_user_data(e);
    menu->flag[i] = lv_obj_has_state(menu->box[i], LV_STATE_CHECKED);
  }

  static void onOk(lv_event_t*) {
    if (!menu) return;
    pend = *menu;
    closeMenu();
    cfgUpdate([](Config& c) {
      c.aprs_sort = pend.sort;
      c.aprs_beacon = pend.flag[0];
      c.aprs_alert_en = pend.flag[1];
      c.aprs_al_near = pend.flag[2];
      c.led_al_en = pend.flag[3];
    });
    if (self_) { self_->sort_ = pend.sort; self_->ver_ = ~g_store.aprs.version(); }
  }

  static void onCancel(lv_event_t*) { closeMenu(); }

  static lv_obj_t* tile(lv_obj_t* p, const char* txt, int x, int y, int w, int h, lv_event_cb_t cb, intptr_t id) {
    lv_obj_t* t = lv_obj_create(p);
    lv_obj_remove_style_all(t);
    lv_obj_set_pos(t, x, y);
    lv_obj_set_size(t, w, h);
    lv_obj_set_style_bg_opa(t, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(t, 3, 0);
    lv_obj_set_style_text_font(t, &font_14, 0);
    lv_obj_remove_flag(t, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* l = lv_label_create(t);
    lv_label_set_text(l, txt);
    lv_obj_center(l);
    lv_obj_add_flag(t, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(t, cb, LV_EVENT_CLICKED, (void*)id);
    return t;
  }

  static void label(lv_obj_t* p, const char* txt, int y) {
    lv_obj_t* l = lv_label_create(p);
    lv_obj_set_style_text_font(l, &font_14, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(0xC0C0C0), 0);
    lv_label_set_text(l, txt);
    lv_obj_set_pos(l, 8, y);
  }

  static void onMenu(lv_event_t*) {
    if (menu) return;
    const Config& c = cfg();
    menu = new Menu{};
    menu->sort = self_->sort_;
    menu->flag[0] = c.aprs_beacon;
    menu->flag[1] = c.aprs_alert_en;
    menu->flag[2] = c.aprs_al_near;
    menu->flag[3] = c.led_al_en;
    lv_obj_t* p = menu->panel = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(p);
    lv_obj_set_size(p, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(p, COL_BG, 0);
    lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(p, COL_TEXT, 0);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(p, LV_OBJ_FLAG_CLICKABLE);  // swallow touches

    label(p, tr(Tr::SortBy), 6);
    const Tr sn[] = {Tr::SortTime, Tr::SortCall, Tr::SortDist};
    for (int i = 0; i < 3; i++) menu->tile[i] = tile(p, tr(sn[i]), 8 + i * 96, 26, 92, 26, onTile, i);
    label(p, tr(Tr::AprsSetting), 66);
    const Tr bn[] = {Tr::Beaconing, Tr::AprsAlert, Tr::RangeAlert, Tr::LedBuzzer};
    for (int i = 0; i < 4; i++) {
      lv_obj_t* cb = menu->box[i] = lv_checkbox_create(p);
      lv_checkbox_set_text(cb, tr(bn[i]));
      lv_obj_set_style_text_font(cb, &font_14, 0);
      lv_obj_set_pos(cb, 24, 88 + i * 28);
      if (menu->flag[i]) lv_obj_add_state(cb, LV_STATE_CHECKED);
      lv_obj_add_event_cb(cb, onBox, LV_EVENT_VALUE_CHANGED, (void*)(intptr_t)i);
    }
    lv_obj_t* ok = tile(p, tr(Tr::Ok), 40, 208, 100, 26, onOk, 0);
    lv_obj_t* cancel = tile(p, tr(Tr::Cancel), 180, 208, 100, 26, onCancel, 0);
    paintTile(ok, true);
    paintTile(cancel, false);
    refreshMenu();
  }

  static bool cell(size_t row, uint8_t col, char* out, size_t cap, void* ctx) {
    auto* s = static_cast<AprsListScreen*>(ctx);
    if (row != s->cachedRow_) {
      g_store.aprs.read([&](const RingBuffer<AprsStation, 30>& r) { s->cached_ = r.at(s->idx_[row]); });
      s->cachedRow_ = row;
    }
    const AprsStation& a = s->cached_;
    switch (col) {
      case 0:
        if (!a.utc) snprintf(out, cap, "--");
        else snprintf(out, cap, "%02u%02u", (unsigned)(a.utc % 86400 / 3600), (unsigned)(a.utc % 3600 / 60));
        break;
      case 1: snprintf(out, cap, "%s", a.call); break;
      case 2: {
        const char* n;
        if (symName(a.sym, n)) snprintf(out, cap, "%s", n);
        else snprintf(out, cap, "%c%c", a.sym[0] ? a.sym[0] : '-', a.sym[1] ? a.sym[1] : ' ');
        break;
      }
      case 3:
        if (a.hasLatLon && a.distKm >= 0) snprintf(out, cap, "%d", (int)a.distKm);
        else snprintf(out, cap, "-");
        break;
      default:
        if (a.freqMhz > 0) snprintf(out, cap, "%.3f", (double)a.freqMhz);
        else snprintf(out, cap, "-");
        break;
    }
    return true;
  }

  void refresh() {
    AprsStation* snap = new AprsStation[30];  // copied under the lock, sorted outside it (heap: 2.5 KB)
    size_t n = 0;
    g_store.aprs.read([&](const RingBuffer<AprsStation, 30>& r) {
      ver_ = g_store.aprs.version();
      for (; n < r.size() && n < 30; n++) snap[n] = r.at(n);
    });
    n_ = n;
    for (size_t i = 0; i < n; i++) idx_[i] = (uint8_t)i;
    if (sort_ != SORT_TIME)
      for (size_t i = 1; i < n; i++) {  // insertion sort, n <= 30; stable
        uint8_t v = idx_[i];
        size_t j = i;
        while (j > 0 && before(snap[v], snap[idx_[j - 1]])) { idx_[j] = idx_[j - 1]; j--; }
        idx_[j] = v;
      }
    delete[] snap;
    lv_obj_set_flag(wait_, LV_OBJ_FLAG_HIDDEN, n_ != 0);
    lv_obj_set_flag(table_, LV_OBJ_FLAG_HIDDEN, n_ == 0);
    cachedRow_ = (size_t)-1;
    spotTableSet(table_, n_, cell, this);
  }

  bool before(const AprsStation& a, const AprsStation& b) const {
    if (sort_ == SORT_CALL) return strcmp(a.call, b.call) < 0;
    bool ak = a.hasLatLon && a.distKm >= 0, bk = b.hasLatLon && b.distKm >= 0;
    if (ak != bk) return ak;  // unknown distances last
    return ak && a.distKm < b.distKm;
  }
};
}  // namespace

Screen* createAprsListScreen() { return new AprsListScreen(); }
