#include "bands_screen.h"

#include "prop_common.h"
#include "../fonts/fonts.h"
#include "../widgets/header.h"
#include "../widgets/ui_kit.h"

namespace {
constexpr int COL_X[3] = {8, 150, 240};

class BandsScreen : public Screen {
 public:
  ScreenId id() const override { return ScreenId::BandInfo; }

  void create(lv_obj_t* p) override {
    const lv_color_t grey = lv_color_hex(0xC0C0C0);
    hdr_ = headerCreate(p, Tr::HfBands);
    uiLabel(p, &font_14, grey, tr(Tr::Band), COL_X[0], 34);
    uiLabel(p, &font_14, grey, tr(Tr::ColDay), COL_X[1], 34);
    uiLabel(p, &font_14, grey, tr(Tr::ColNight), COL_X[2], 34);
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
      for (int c = 0; c < 3; c++) cell_[i][c] = uiLabel(p, &font_14, COL_TEXT, "", COL_X[c], y + 6);
    }
    uiLabel(p, &font_14, grey, tr(Tr::UpdatedUtc), 10, 182);
    upd_ = uiLabel(p, &font_14, COL_TEXT, "--", 10, 200);
    age_ = uiLabel(p, &font_14, lv_color_hex(0x808080), "", 10, 222);
    err_ = uiLabel(p, &font_14, COL_ERROR, tr(Tr::NoData), 150, 182);
    refresh(0);
  }

  void tick(uint32_t now) override {
    headerTickUtc(hdr_);
    refresh(now);
  }

 private:
  lv_obj_t *hdr_, *cell_[4][3], *upd_, *age_, *err_;
  PropFeed feed_;

  void refresh(uint32_t now) {
    feed_.refresh(now, upd_, age_, err_, [&](const PropagationData& d) {
      static const char* const labels[4] = {"80m-40m", "30m-20m", "17m-15m", "12m-10m"};
      for (int i = 0; i < 4; i++) {
        const char* t[3] = {d.valid ? d.band[i].label : labels[i], d.valid ? d.band[i].day : "--",
                            d.valid ? d.band[i].night : "--"};
        for (int c = 0; c < 3; c++) {
          lv_label_set_text(cell_[i][c], c ? propCondText(t[c]) : t[c]);
          lv_obj_set_style_text_color(cell_[i][c], c ? propCondColor(t[c]) : COL_TEXT, 0);
        }
      }
    });
  }
};
}  // namespace

Screen* createBandsScreen() { return new BandsScreen(); }
