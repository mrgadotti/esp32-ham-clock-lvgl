#include "spot_list_screen.h"

#include <stdio.h>
#include <string.h>

#include "band_mode.h"
#include "i18n.h"
#include "text_cut.h"
#include "../../app/config_store.h"
#include "../theme.h"
#include "../widgets/filter_menu.h"
#include "../widgets/header.h"
#include "../widgets/spot_table.h"

namespace {
constexpr uint32_t kRefreshMs = 1000;  // table rebuild is costly (up to 500 cells): at most 1 Hz
constexpr size_t kTextMax = 15;        // the last column fits ~14 characters; shorter cell text saves LV_MEM

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
    const int16_t w[] = {40, 76, 60, 42, 102};
    const lv_color_t c[] = {lv_color_hex(0xC0C0C0), COL_TEXT, COL_FREQ, COL_MODE, COL_ACCENT};
    for (int i = 0; i < 5; i++) { cols_.width[i] = w[i]; cols_.color[i] = c[i]; }
    const Tr hdr[] = {Tr::Utc, Tr::ColCall, Tr::Mhz, Tr::ColMod, d_.comment ? Tr::ColMsg : Tr::ColCountry};
    ui_ = spotListUiCreate(p, d_.title, cols_, hdr, Tr::WaitingForSpots, onFilter);
    force_ = true;
  }

  void tick(uint32_t) override {
    bool noLogin = d_.id == ScreenId::HamAlert && (!cfg().hama_user[0] || !cfg().hama_pass[0]);
    if (noLogin != noLogin_) {
      noLogin_ = noLogin;
      lv_label_set_text(ui_.wait, tr(noLogin ? Tr::HamAlertNoLogin : Tr::WaitingForSpots));
    }
    uint32_t now = lv_tick_get();
    if (force_ || (ver_ != d_.src.version(d_.src.store) && now - lastMs_ >= kRefreshMs)) refresh(now);
    headerTickUtc(ui_.hdr);
  }

 private:
  static inline SpotListScreen* self_ = nullptr;
  const SpotListDef d_;
  SpotTableCols cols_{};
  SpotListUi ui_{};
  uint8_t mode_;
  uint16_t band_;
  uint32_t ver_ = 0, lastMs_ = 0;
  bool force_ = false, noLogin_ = false;
  uint8_t idx_[SPOT_LIST_MAX];  // filtered position -> store index
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
    self_->force_ = true;  // rebuild now with the new filter
  }

  static bool cell(size_t row, uint8_t col, char* out, size_t cap, void* ctx) {
    auto* s = static_cast<SpotListScreen*>(ctx);
    if (row != s->cachedRow_) {  // one lock + copy per row, reused for its cells
      s->d_.src.get(s->d_.src.store, s->idx_[row], s->cached_);
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
      default: {
        const char* t = s->d_.comment ? sp.comment : sp.country;
        textCut(out, cap, t[0] ? t : "-", kTextMax);
        break;
      }
    }
    return true;
  }

  void refresh(uint32_t now) {
    force_ = false;
    lastMs_ = now;
    n_ = d_.src.filter(d_.src.store, idx_, sizeof idx_, band_, mode_, ver_);
    cachedRow_ = (size_t)-1;
    spotListUiSet(ui_, n_, cell, this);
  }
};
}  // namespace

Screen* createSpotListScreen(const SpotListDef& def) { return new SpotListScreen(def); }
