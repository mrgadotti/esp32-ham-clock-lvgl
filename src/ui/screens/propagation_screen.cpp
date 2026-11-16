#include "propagation_screen.h"

#include <stdlib.h>

#include "prop_common.h"
#include "../../app/data_store.h"
#include "../fonts/fonts.h"
#include "../widgets/header.h"

namespace {
class PropagationScreen : public Screen {
 public:
  ScreenId id() const override { return ScreenId::SunSpots; }

  void create(lv_obj_t* p) override {
    hdr_ = headerCreate(p, Tr::Propagation);
    static const Tr names[4] = {Tr::Sfi, Tr::KIndex, Tr::AIndex, Tr::Muf};
    for (int i = 0; i < 4; i++) {
      int y = HEADER_H + 4 + i * 38;
      lv_obj_t* l = lv_label_create(p);
      lv_obj_set_style_text_font(l, &font_14, 0);
      lv_obj_set_style_text_color(l, lv_color_hex(0xC0C0C0), 0);
      lv_label_set_text(l, tr(names[i]));
      lv_obj_set_pos(l, 10, y + 12);
      val_[i] = lv_label_create(p);
      lv_obj_set_style_text_font(val_[i], &font_24, 0);
      lv_obj_set_width(val_[i], 200);
      lv_obj_set_style_text_align(val_[i], LV_TEXT_ALIGN_RIGHT, 0);
      lv_obj_set_pos(val_[i], 110, y);
    }
    lv_obj_t* u = lv_label_create(p);
    lv_obj_set_style_text_font(u, &font_14, 0);
    lv_obj_set_style_text_color(u, lv_color_hex(0xC0C0C0), 0);
    lv_label_set_text(u, tr(Tr::UpdatedUtc));
    lv_obj_set_pos(u, 10, 188);
    upd_ = lv_label_create(p);
    lv_obj_set_style_text_font(upd_, &font_14, 0);
    lv_obj_set_pos(upd_, 10, 204);
    age_ = lv_label_create(p);
    lv_obj_set_style_text_font(age_, &font_14, 0);
    lv_obj_set_style_text_color(age_, lv_color_hex(0x808080), 0);
    lv_obj_set_pos(age_, 10, 222);
    err_ = lv_label_create(p);
    lv_obj_set_style_text_font(err_, &font_14, 0);
    lv_obj_set_style_text_color(err_, COL_ERROR, 0);
    lv_label_set_text(err_, tr(Tr::NoData));
    lv_obj_set_pos(err_, 10, 170);
    ver_ = ~0u;
    refresh(0);
  }

  void tick(uint32_t now) override {
    headerTickUtc(hdr_);
    refresh(now);
  }

 private:
  lv_obj_t *hdr_, *val_[4], *upd_, *age_, *err_;
  uint32_t ver_, ageAt_ = 0;
  PropagationData d_{};

  void refresh(uint32_t now) {
    bool changed = g_store.prop.version() != ver_;
    if (changed) {
      ver_ = g_store.prop.version();
      g_store.prop.read([&](const PropagationData& s) { d_ = s; });
      const char* v[4] = {d_.sfi, d_.kindex, d_.aindex, d_.muf};
      lv_color_t col[4] = {lv_color_hex(0x00FF00), kColor(), lv_color_hex(0x00FFFF), lv_color_hex(0xFFFF00)};
      for (int i = 0; i < 4; i++) {
        lv_label_set_text(val_[i], d_.valid ? v[i] : "--");
        lv_obj_set_style_text_color(val_[i], col[i], 0);
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

  lv_color_t kColor() const {
    if (!d_.valid) return COL_TEXT;
    int k = atoi(d_.kindex);
    return lv_color_hex(k < 4 ? 0x00FF00 : k < 6 ? 0xFF8C00 : 0xFF0000);
  }
};
}  // namespace

Screen* createPropagationScreen() { return new PropagationScreen(); }
