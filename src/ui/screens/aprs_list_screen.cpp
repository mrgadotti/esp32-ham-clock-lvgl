#include "aprs_list_screen.h"

#include <stdio.h>
#include <string.h>

#include "i18n.h"
#include "aprs_sort.h"
#include "text_cut.h"
#include "../../app/config_store.h"
#include "../../app/data_store.h"
#include "../../svc/aprs_beacon.h"
#include "../fonts/fonts.h"
#include "../theme.h"
#include "../widgets/header.h"
#include "../widgets/spot_table.h"
#include "../widgets/ui_kit.h"

namespace {
struct Menu {
  lv_obj_t* panel;
  lv_obj_t* tile[3];
  lv_obj_t* box[4];
  int sort;
  bool flag[4];  // beacon, alert, range, led
};
class AprsListScreen : public Screen {
 public:
  ~AprsListScreen() override {
    closeMenu();
    if (self_ == this) self_ = nullptr;
  }
  ScreenId id() const override { return ScreenId::AprsList; }
  bool blocksSwipe() const override { return menu_ != nullptr; }

  void create(lv_obj_t* p) override {
    self_ = this;
    sort_ = (int)cfg().aprs_sort;
    if (sort_ < 0 || sort_ > APRS_SORT_DIST) sort_ = APRS_SORT_TIME;
    cols_.n = 5;
    const int16_t w[] = {41, 93, 43, 58, 85};  // UTC CALL KM FREQ MSG, sized by tools/check_text_fit.py
    const lv_color_t c[] = {COL_GREY, COL_TEXT, COL_ACCENT, COL_FREQ, COL_CYAN};
    for (int i = 0; i < 5; i++) { cols_.width[i] = w[i]; cols_.color[i] = c[i]; }
    const char* cm = cfg().uart2_cmod;
    Tr title = !strcmp(cm, "aprs_tnc_kiss") ? Tr::AprsKiss : !strcmp(cm, "aprs_tnc") ? Tr::AprsTnc2 : Tr::AprsIs;
    title_ = title;
    const Tr hd[] = {Tr::Utc, Tr::ColCall, Tr::ColKm, Tr::ColFreq, Tr::ColMsg};
    ui_ = spotListUiCreate(p, title, cols_, hd, Tr::WaitingForAprs, onMenu);
    cfgVer_ = cfgVersion();
    updateAprsIsTitle();
    force_ = true;
  }

  void tick(uint32_t) override {
    uint32_t now = lv_tick_get();
    if (force_ || (ver_ != g_store.aprs.version() && now - lastMs_ >= 1000)) refresh(now);  // table rebuild: <= 1 Hz
    headerTickUtc(ui_.hdr);
    if (spotListUiTimeSync(ui_)) force_ = true;
    if (cfgVersion() != cfgVer_) {
      cfgVer_ = cfgVersion();  // read before the config: a racing update only costs one extra refresh
      updateAprsIsTitle();
    }
  }

 private:
  static inline AprsListScreen* self_ = nullptr;
  SpotTableCols cols_{};
  SpotListUi ui_{};
  int sort_;
  Tr title_ = Tr::AprsIs;
  uint32_t ver_ = 0, lastMs_ = 0, cfgVer_ = 0;
  bool force_ = false;
  uint8_t idx_[30];
  size_t n_ = 0;
  size_t cachedRow_ = (size_t)-1;
  AprsStation cached_;

  Menu* menu_ = nullptr;

  void closeMenu() {
    if (!menu_) return;
    lv_obj_delete_async(menu_->panel);  // may run inside a widget's own callback
    delete menu_;
    menu_ = nullptr;
  }

  static void refreshMenu() {
    for (int i = 0; i < 3; i++) uiTilePaint(self_->menu_->tile[i], self_->menu_->sort == i);
  }

  static void onTile(lv_event_t* e) {
    if (!self_ || !self_->menu_) return;
    self_->menu_->sort = (int)(intptr_t)lv_event_get_user_data(e);
    refreshMenu();
  }

  static void onBox(lv_event_t* e) {
    if (!self_ || !self_->menu_) return;
    int i = (int)(intptr_t)lv_event_get_user_data(e);
    self_->menu_->flag[i] = lv_obj_has_state(self_->menu_->box[i], LV_STATE_CHECKED);
  }

  static void onOk(lv_event_t*) {
    if (!self_ || !self_->menu_) return;
    const Menu pend = *self_->menu_;  // closeMenu() frees the menu
    self_->closeMenu();
    cfgUpdate([pend](Config& c) {
      c.aprs_sort = pend.sort;
      c.aprs_beacon = pend.flag[0];
      c.aprs_alert_en = pend.flag[1];
      c.aprs_al_near = pend.flag[2];
      c.led_al_en = pend.flag[3];
    });
    if (self_) { self_->sort_ = pend.sort; self_->force_ = true; }
  }

  static void onCancel(lv_event_t*) { if (self_) self_->closeMenu(); }

  static void onBeacon(lv_event_t*) {
    if (self_) self_->closeMenu();
    aprsBeaconRequest();
  }

  static void onClear(lv_event_t*) {
    if (self_) self_->closeMenu();
    g_store.aprs.write([](AprsRing& r) { r.clear(); });
  }

  // Appends the APRS-IS login identity to the header title, matching aprs_is.cpp ("user <call>[-<ssid>]").
  // TNC2/KISS titles are left untouched. Reads the UI snapshot; never allocates.
  void updateAprsIsTitle() {
    if (title_ != Tr::AprsIs) return;
    const Config& c = cfg();
    char buf[64];
    if (c.aprs_callsign[0]) {
      if (c.aprs_ssid > 0)
        snprintf(buf, sizeof buf, "%s: %s-%d", tr(Tr::AprsIs), c.aprs_callsign, (int)c.aprs_ssid);
      else
        snprintf(buf, sizeof buf, "%s: %s", tr(Tr::AprsIs), c.aprs_callsign);
    } else {
      snprintf(buf, sizeof buf, "%s", tr(Tr::AprsIs));
    }
    lv_obj_t* label = headerTitle(ui_.hdr);
    if (strcmp(lv_label_get_text(label), buf) != 0) lv_label_set_text(label, buf);
  }

  static void onMenu(lv_event_t*) {
    if (self_->menu_) return;
    const Config& c = cfg();
    self_->menu_ = new Menu{};
    self_->menu_->sort = self_->sort_;
    self_->menu_->flag[0] = c.aprs_beacon;
    self_->menu_->flag[1] = c.aprs_alert_en;
    self_->menu_->flag[2] = c.aprs_al_near;
    self_->menu_->flag[3] = c.led_al_en;
    lv_obj_t* p = self_->menu_->panel = uiModalPanel();

    uiLabel(p, &font_14, COL_GREY, tr(Tr::SortBy), 8, 6);
    const Tr sn[] = {Tr::SortTime, Tr::SortCall, Tr::SortDist};
    for (int i = 0; i < 3; i++) self_->menu_->tile[i] = uiTile(p, tr(sn[i]), 8 + i * 96, 26, 92, 26, onTile, i);
    uiLabel(p, &font_14, COL_GREY, tr(Tr::AprsSetting), 8, 58);
    const Tr bn[] = {Tr::Beaconing, Tr::AprsAlert, Tr::RangeAlert, Tr::LedBuzzer};
    lv_obj_t* area = uiScrollArea(p, 0, 76, SCREEN_W, 92);  // 4 rows x 32 px = 128 px: scrolls
    for (int i = 0; i < 4; i++) {
      lv_obj_t* cb = self_->menu_->box[i] = lv_checkbox_create(area);
      lv_checkbox_set_text(cb, tr(bn[i]));
      lv_obj_set_style_text_font(cb, &font_14, 0);
      lv_obj_set_height(cb, 30);  // 32 px pitch: touch areas never overlap
      lv_obj_set_pos(cb, 24, i * 32);
      if (self_->menu_->flag[i]) lv_obj_add_state(cb, LV_STATE_CHECKED);
      lv_obj_add_event_cb(cb, onBox, LV_EVENT_VALUE_CHANGED, (void*)(intptr_t)i);
    }
    uiTilePaint(uiTile(p, tr(Tr::BeaconNow), 8, 172, 148, 26, onBeacon, 0), true);
    uiTilePaint(uiTile(p, tr(Tr::ClearAprsList), 164, 172, 148, 26, onClear, 0), true);
    lv_obj_t* ok = uiTile(p, tr(Tr::Ok), 40, 208, 100, 26, onOk, 0);
    lv_obj_t* cancel = uiTile(p, tr(Tr::Cancel), 180, 208, 100, 26, onCancel, 0);
    uiTilePaint(ok, true);
    uiTilePaint(cancel, true);
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
      g_store.aprs.read([&](const AprsRing& r) { s->cached_ = r.at(s->idx_[row]); });
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
    g_store.aprs.read([&](const AprsRing& r) {
      ver_ = g_store.aprs.version();
      n_ = r.size() < sizeof idx_ ? r.size() : sizeof idx_;
      for (size_t i = 0; i < n_; i++) idx_[i] = (uint8_t)i;
      if (sort_ != APRS_SORT_TIME)
        for (size_t i = 1; i < n_; i++) {
          uint8_t v = idx_[i];
          size_t j = i;
          while (j > 0 && aprsBefore(r.at(v), r.at(idx_[j - 1]), sort_)) { idx_[j] = idx_[j - 1]; j--; }
          idx_[j] = v;
        }
    });
    cachedRow_ = (size_t)-1;
    spotListUiSet(ui_, n_, cell, this);
  }
};
}  // namespace

Screen* createAprsListScreen() { return new AprsListScreen(); }
