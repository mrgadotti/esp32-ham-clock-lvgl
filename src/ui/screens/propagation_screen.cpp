#include "propagation_screen.h"

#include "prop_common.h"
#include "../fonts/fonts.h"
#include "../widgets/header.h"
#include "../widgets/ui_kit.h"

namespace {
class PropagationScreen : public Screen {
 public:
  ScreenId id() const override { return ScreenId::SunSpots; }

  void create(lv_obj_t* p) override {
    hdr_ = headerCreate(p, Tr::Propagation);
    static const Tr names[4] = {Tr::Sfi, Tr::KIndex, Tr::AIndex, Tr::Muf};
    const lv_color_t grey = lv_color_hex(0xC0C0C0);
    for (int i = 0; i < 4; i++) {
      int y = HEADER_H + 4 + i * 38;
      uiLabel(p, &font_14, grey, tr(names[i]), 10, y + 12);
      val_[i] = uiLabel(p, &font_24, COL_TEXT, "", 110, y, 200, LV_TEXT_ALIGN_RIGHT);
    }
    uiLabel(p, &font_14, grey, tr(Tr::UpdatedUtc), 10, 186);
    upd_ = uiLabel(p, &font_14, COL_TEXT, "", 10, 204);
    age_ = uiLabel(p, &font_14, lv_color_hex(0x808080), "", 10, 222);
    err_ = uiLabel(p, &font_14, COL_ERROR, tr(Tr::NoData), 10, 172);
    refresh(0);
  }

  void tick(uint32_t now) override {
    headerTickUtc(hdr_);
    refresh(now);
  }

 private:
  lv_obj_t *hdr_, *val_[4], *upd_, *age_, *err_;
  PropFeed feed_;

  void refresh(uint32_t now) {
    feed_.refresh(now, upd_, age_, err_, [&](const PropagationData& d) {
      const char* v[4] = {d.sfi, d.kindex, d.aindex, d.muf};
      lv_color_t col[4] = {lv_color_hex(0x00FF00), kColor(d), lv_color_hex(0x00FFFF), lv_color_hex(0xFFFF00)};
      for (int i = 0; i < 4; i++) {
        lv_label_set_text(val_[i], d.valid ? v[i] : "--");
        lv_obj_set_style_text_color(val_[i], col[i], 0);
      }
    });
  }

  static lv_color_t kColor(const PropagationData& d) {
    if (!d.valid) return COL_TEXT;
    switch (kIndexLevel(d.kindex)) {
      case KLevel::Low: return lv_color_hex(0x00FF00);
      case KLevel::Mid: return lv_color_hex(0xFF8C00);
      case KLevel::High: return lv_color_hex(0xFF0000);
      default: return COL_TEXT;  // non-numeric: neutral
    }
  }
};
}  // namespace

Screen* createPropagationScreen() { return new PropagationScreen(); }
