#include "aprs_list_screen.h"

#include <stdio.h>
#include <string.h>

#include "i18n.h"
#include "text_cut.h"
#include "../../app/config_store.h"
#include "../../app/data_store.h"
#include "../fonts/fonts.h"
#include "../theme.h"
#include "../widgets/header.h"
#include "../widgets/spot_table.h"
#include "../widgets/ui_kit.h"

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
    const int16_t w[] = {41, 93, 43, 58, 85};  // UTC CALL KM FREQ MSG, sized by tools/check_text_fit.py
    const lv_color_t c[] = {lv_color_hex(0xC0C0C0), COL_TEXT, COL_ACCENT, COL_FREQ, lv_color_hex(0x00FFFF)};
    for (int i = 0; i < 5; i++) { cols_.width[i] = w[i]; cols_.color[i] = c[i]; }
    const char* cm = cfg().uart2_cmod;
    Tr title = !strcmp(cm, "aprs_tnc_kiss") ? Tr::AprsKiss : !strcmp(cm, "aprs_tnc") ? Tr::AprsTnc2 : Tr::AprsIs;
    const Tr hd[] = {Tr::Utc, Tr::ColCall, Tr::ColKm, Tr::ColFreq, Tr::ColMsg};
    ui_ = spotListUiCreate(p, title, cols_, hd, Tr::WaitingForAprs, onMenu);
    force_ = true;
  }

  void tick(uint32_t) override {
    uint32_t now = lv_tick_get();
    if (force_ || (ver_ != g_store.aprs.version() && now - lastMs_ >= 1000)) refresh(now);  // table rebuild: <= 1 Hz
    headerTickUtc(ui_.hdr);
    if (spotListUiTimeSync(ui_)) force_ = true;
  }

 private:
  static inline AprsListScreen* self_ = nullptr;
  SpotTableCols cols_{};
  SpotListUi ui_{};
  int sort_;
  uint32_t ver_ = 0, lastMs_ = 0;
  bool force_ = false;
  uint8_t idx_[30];
  size_t n_ = 0;
  size_t cachedRow_ = (size_t)-1;
  AprsStation cached_;

  static void refreshMenu() {
    for (int i = 0; i < 3; i++) uiTilePaint(menu->tile[i], menu->sort == i);
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
    const Menu pend = *menu;  // closeMenu() frees *menu
    closeMenu();
    cfgUpdate([pend](Config& c) {
      c.aprs_sort = pend.sort;
      c.aprs_beacon = pend.flag[0];
      c.aprs_alert_en = pend.flag[1];
      c.aprs_al_near = pend.flag[2];
      c.led_al_en = pend.flag[3];
    });
    if (self_) { self_->sort_ = pend.sort; self_->force_ = true; }
  }

  static void onCancel(lv_event_t*) { closeMenu(); }

  static void onMenu(lv_event_t*) {
    if (menu) return;
    const Config& c = cfg();
    menu = new Menu{};
    menu->sort = self_->sort_;
    menu->flag[0] = c.aprs_beacon;
    menu->flag[1] = c.aprs_alert_en;
    menu->flag[2] = c.aprs_al_near;
    menu->flag[3] = c.led_al_en;
    lv_obj_t* p = menu->panel = uiModalPanel();

    uiLabel(p, &font_14, lv_color_hex(0xC0C0C0), tr(Tr::SortBy), 8, 6);
    const Tr sn[] = {Tr::SortTime, Tr::SortCall, Tr::SortDist};
    for (int i = 0; i < 3; i++) menu->tile[i] = uiTile(p, tr(sn[i]), 8 + i * 96, 26, 92, 26, onTile, i);
    uiLabel(p, &font_14, lv_color_hex(0xC0C0C0), tr(Tr::AprsSetting), 8, 66);
    const Tr bn[] = {Tr::Beaconing, Tr::AprsAlert, Tr::RangeAlert, Tr::LedBuzzer};
    for (int i = 0; i < 4; i++) {
      lv_obj_t* cb = menu->box[i] = lv_checkbox_create(p);
      lv_checkbox_set_text(cb, tr(bn[i]));
      lv_obj_set_style_text_font(cb, &font_14, 0);
      lv_obj_set_pos(cb, 24, 88 + i * 28);
      if (menu->flag[i]) lv_obj_add_state(cb, LV_STATE_CHECKED);
      lv_obj_add_event_cb(cb, onBox, LV_EVENT_VALUE_CHANGED, (void*)(intptr_t)i);
    }
    lv_obj_t* ok = uiTile(p, tr(Tr::Ok), 40, 208, 100, 26, onOk, 0);
    lv_obj_t* cancel = uiTile(p, tr(Tr::Cancel), 180, 208, 100, 26, onCancel, 0);
    uiTilePaint(ok, true);
    uiTilePaint(cancel, false);
    refreshMenu();
  }

  // Copies s to out, dropping whole characters and appending "..." until the text fits maxW px.
  static void ellipsize(char* out, size_t cap, const char* s, int maxW) {
    if (!s[0]) { snprintf(out, cap, "-"); return; }
    textCut(out, cap, s, cap - 4);  // room for the "..."
    lv_point_t sz;
    lv_text_get_size(&sz, out, &font_14, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    if (sz.x <= maxW && !s[strlen(out)]) return;
    size_t n = strlen(out);
    while (n) {
      do n--; while (n && ((uint8_t)out[n] & 0xC0) == 0x80);  // back to a character boundary
      out[n] = 0;
      strcat(out, "...");
      lv_text_get_size(&sz, out, &font_14, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
      if (sz.x <= maxW) return;
      out[n] = 0;
    }
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
        formatHmCell(out, cap, a.utc, uiTimeOffsetSec());
        break;
      case 1: snprintf(out, cap, "%s", a.call); break;
      case 2:
        if (a.hasLatLon && a.distKm >= 0) formatKmCell(out, cap, (int)a.distKm);
        else snprintf(out, cap, "-");
        break;
      case 3:
        if (a.freqMhz > 0) snprintf(out, cap, "%.3f", (double)a.freqMhz);
        else snprintf(out, cap, "-");
        break;
      default: ellipsize(out, cap, a.comment, s->cols_.width[4] - SPOT_TABLE_PAD_L - 1); break;
    }
    return true;
  }

  // Snapshot index + sort under the lock (n <= 30, insertion sort, stable): no heap copy of the stations.
  void refresh(uint32_t now) {
    force_ = false;
    lastMs_ = now;
    g_store.aprs.read([&](const RingBuffer<AprsStation, 30>& r) {
      ver_ = g_store.aprs.version();
      n_ = r.size() < sizeof idx_ ? r.size() : sizeof idx_;
      for (size_t i = 0; i < n_; i++) idx_[i] = (uint8_t)i;
      if (sort_ != SORT_TIME)
        for (size_t i = 1; i < n_; i++) {
          uint8_t v = idx_[i];
          size_t j = i;
          while (j > 0 && before(r.at(v), r.at(idx_[j - 1]))) { idx_[j] = idx_[j - 1]; j--; }
          idx_[j] = v;
        }
    });
    cachedRow_ = (size_t)-1;
    spotListUiSet(ui_, n_, cell, this);
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
