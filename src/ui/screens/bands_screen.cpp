#include "bands_screen.h"

#include "prop_common.h"
#include "../../app/data_store.h"
#include "../fonts/fonts.h"
#include "../widgets/header.h"

namespace {
constexpr int COL_X[3] = {8, 150, 240};

class BandsScreen : public Screen {
 public:
  ScreenId id() const override { return ScreenId::BandInfo; }

  void create(lv_obj_t* p) override {
    hdr_ = headerCreate(p, Tr::HfBands);
    cell(p, COL_X[0], 34, tr(Tr::Band), lv_color_hex(0xC0C0C0));
    cell(p, COL_X[1], 34, tr(Tr::ColDay), lv_color_hex(0xC0C0C0));
    cell(p, COL_X[2], 34, tr(Tr::ColNight), lv_color_hex(0xC0C0C0));
    for (int i = 0; i < 4; i++) {
      int y = 56 + i * 30;
      if (i % 2 == 0) {  // zebra
        lv_obj_t* r = lv_obj_create(p);
        lv_obj_remove_style_all(r);
        lv_obj_set_size(r, 320, 28);
        lv_obj_set_pos(r, 0, y);
        lv_obj_set_style_bg_color(r, lv_color_hex(0x181818), 0);
        lv_obj_set_style_bg_opa(r, LV_OPA_COVER, 0);
      }
      for (int c = 0; c < 3; c++) cell_[i][c] = cell(p, COL_X[c], y + 6, "", COL_TEXT);
    }
    cell(p, 10, 182, tr(Tr::UpdatedUtc), lv_color_hex(0xC0C0C0));
    upd_ = cell(p, 10, 200, "--", COL_TEXT);
    age_ = cell(p, 10, 222, "", lv_color_hex(0x808080));
    err_ = cell(p, 150, 182, tr(Tr::NoData), COL_ERROR);
    ver_ = ~0u;
    refresh(0);
  }

  void tick(uint32_t now) override {
    headerTickUtc(hdr_);
    refresh(now);
  }

 private:
  lv_obj_t *hdr_, *cell_[4][3], *upd_, *age_, *err_;
  uint32_t ver_, ageAt_ = 0;
  PropagationData d_{};

  static lv_obj_t* cell(lv_obj_t* p, int x, int y, const char* t, lv_color_t col) {
    lv_obj_t* l = lv_label_create(p);
    lv_obj_set_style_text_font(l, &font_14, 0);
    lv_obj_set_style_text_color(l, col, 0);
    lv_label_set_text(l, t);
    lv_obj_set_pos(l, x, y);
    return l;
  }

  void refresh(uint32_t now) {
    bool changed = g_store.prop.version() != ver_;
    if (changed) {
      ver_ = g_store.prop.version();
      g_store.prop.read([&](const PropagationData& s) { d_ = s; });
      static const char* const labels[4] = {"80m-40m", "30m-20m", "17m-15m", "12m-10m"};
      for (int i = 0; i < 4; i++) {
        const char* t[3] = {d_.valid ? d_.band[i].label : labels[i], d_.valid ? d_.band[i].day : "--",
                            d_.valid ? d_.band[i].night : "--"};
        for (int c = 0; c < 3; c++) {
          lv_label_set_text(cell_[i][c], c ? propCondText(t[c]) : t[c]);
          lv_obj_set_style_text_color(cell_[i][c], c ? propCondColor(t[c]) : COL_TEXT, 0);
        }
      }
      lv_label_set_text(upd_, d_.valid ? d_.updated : "--");
      lv_obj_set_flag(err_, LV_OBJ_FLAG_HIDDEN, d_.valid);
    }
    if (changed || now - ageAt_ >= 30000) {
      ageAt_ = now;
      char a[40];
      propAgeText(d_.fetchedAt, a, sizeof a);
      lv_label_set_text(age_, a);
    }
  }
};
}  // namespace

Screen* createBandsScreen() { return new BandsScreen(); }
