#include "spot_list_screen.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "band_mode.h"
#include "i18n.h"
#include "../../app/config_store.h"
#include "../../app/data_store.h"
#include "../../net/time_service.h"
#include "../fonts/fonts.h"
#include "../theme.h"
#include "../widgets/filter_menu.h"
#include "../widgets/header.h"
#include "../widgets/spot_table.h"

namespace {
int32_t pendMode, pendBand;  // cfgUpdate takes a plain function pointer
const SpotListDef* pendDef;

class SpotListScreen : public Screen {
 public:
  explicit SpotListScreen(const SpotListDef& d) : d_(d) {}
  ~SpotListScreen() override {
    filterMenuClose();
    if (self_ == this) self_ = nullptr;
  }
  ScreenId id() const override { return d_.id; }
  bool blocksSwipe() const override { return filterMenuIsOpen(); }

  void create(lv_obj_t* p) override {
    self_ = this;
    mode_ = (uint8_t)(cfg().*d_.modeKey);
    band_ = (uint16_t)(cfg().*d_.bandKey);
    cols_.n = 5;
    const int16_t w[] = {38, 76, 60, 42, 104};
    const lv_color_t c[] = {lv_color_hex(0xC0C0C0), COL_TEXT, COL_FREQ, COL_MODE, COL_ACCENT};
    for (int i = 0; i < 5; i++) { cols_.width[i] = w[i]; cols_.color[i] = c[i]; }
    hdr_ = headerCreate(p, d_.title);
    title_ = lv_obj_get_child(hdr_, 0);  // headerCreate adds the title first
    lv_obj_align(title_, LV_ALIGN_LEFT_MID, 46, 0);
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
    lv_obj_add_event_cb(btn, onFilter, LV_EVENT_CLICKED, nullptr);

    static const Tr kHdr[] = {Tr::Utc, Tr::ColCall, Tr::Mhz, Tr::ColMod, Tr::ColCountry};
    int x = 0;
    for (int i = 0; i < 5; i++) {
      lv_obj_t* l = lv_label_create(p);
      lv_obj_set_style_text_font(l, &font_14, 0);
      lv_obj_set_style_text_color(l, lv_color_hex(0xA0A0A0), 0);
      lv_label_set_text(l, tr(kHdr[i]));
      lv_obj_set_pos(l, x + 2, HEADER_H);
      x += cols_.width[i];
    }
    table_ = spotTableCreate(p, cols_);
    lv_obj_set_pos(table_, 0, HEADER_H + 18);
    lv_obj_set_height(table_, 240 - HEADER_H - 18);
    wait_ = lv_label_create(p);
    lv_obj_set_style_text_font(wait_, &font_18, 0);
    lv_obj_set_style_text_color(wait_, COL_ERROR, 0);
    lv_label_set_text(wait_, tr(Tr::WaitingForSpots));
    lv_obj_center(wait_);
    ver_ = ~d_.store->version();
    refresh();
  }

  void tick(uint32_t) override {
    bool noLogin = d_.id == ScreenId::HamAlert && (!cfg().hama_user[0] || !cfg().hama_pass[0]);
    if (noLogin != noLogin_) {
      noLogin_ = noLogin;
      lv_label_set_text(wait_, tr(noLogin ? Tr::HamAlertNoLogin : Tr::WaitingForSpots));
    }
    if (ver_ != d_.store->version()) refresh();
    if (timeValid()) {
      struct tm tm;
      time_t t = time(nullptr);
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
  static inline SpotListScreen* self_ = nullptr;
  const SpotListDef d_;
  SpotTableCols cols_{};
  lv_obj_t *hdr_, *title_, *table_, *wait_;
  uint8_t mode_;
  uint16_t band_;
  uint32_t ver_;
  int lastMin_ = -1;
  bool noLogin_ = false;
  uint8_t idx_[30];  // filtered position -> store index
  size_t n_ = 0;
  size_t cachedRow_ = (size_t)-1;
  Spot cached_;

  static void onFilter(lv_event_t*) {
    if (!self_) return;
    filterMenuOpen(nullptr, self_->d_.bands, self_->d_.nBands, &self_->band_, &self_->mode_, onSave);
  }

  static void onSave() {
    if (!self_) return;
    pendMode = self_->mode_;
    pendBand = self_->band_;
    pendDef = &self_->d_;
    cfgUpdate([](Config& c) { c.*pendDef->modeKey = pendMode; c.*pendDef->bandKey = pendBand; });
    self_->ver_ = ~self_->d_.store->version();  // force a rebuild with the new filter
  }

  static bool cell(size_t row, uint8_t col, char* out, size_t cap, void* ctx) {
    auto* s = static_cast<SpotListScreen*>(ctx);
    if (row != s->cachedRow_) {  // one lock + copy per row, reused for its cells
      s->d_.store->read([&](const RingBuffer<Spot, 30>& r) { s->cached_ = r.at(s->idx_[row]); });
      s->cachedRow_ = row;
    }
    const Spot& sp = s->cached_;
    switch (col) {
      case 0:
        if (!sp.utc) snprintf(out, cap, "--");
        else snprintf(out, cap, "%02u%02u", (unsigned)(sp.utc % 86400 / 3600), (unsigned)(sp.utc % 3600 / 60));
        break;
      case 1: snprintf(out, cap, "%s", sp.call); break;
      case 2: snprintf(out, cap, "%u.%03u", (unsigned)(sp.khz / 1000), (unsigned)(sp.khz % 1000)); break;
      case 3: snprintf(out, cap, "%s", sp.mode == MODE_CW ? "CW" : sp.mode == MODE_SSB ? "SSB" : sp.mode == MODE_DIGI ? "DIGI" : "-"); break;
      default: snprintf(out, cap, "%s", sp.country[0] ? sp.country : "-"); break;
    }
    return true;
  }

  void refresh() {
    n_ = 0;
    d_.store->read([&](const RingBuffer<Spot, 30>& r) {
      ver_ = d_.store->version();
      for (size_t i = 0; i < r.size() && n_ < sizeof idx_; i++)
        if (passesFilter((Band)r.at(i).band, (Mode)r.at(i).mode, band_, mode_)) idx_[n_++] = (uint8_t)i;
    });
    lv_obj_set_flag(wait_, LV_OBJ_FLAG_HIDDEN, n_ != 0);
    lv_obj_set_flag(table_, LV_OBJ_FLAG_HIDDEN, n_ == 0);
    cachedRow_ = (size_t)-1;
    spotTableSet(table_, n_, cell, this);
  }
};
}  // namespace

Screen* createSpotListScreen(const SpotListDef& def) { return new SpotListScreen(def); }
