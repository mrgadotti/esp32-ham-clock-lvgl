#include "ui_kit.h"

#include "../fonts/fonts.h"
#include "../theme.h"
#include "header.h"

lv_obj_t* uiLabel(lv_obj_t* parent, const lv_font_t* font, lv_color_t color, const char* text, int x, int y) {
  lv_obj_t* l = lv_label_create(parent);
  lv_obj_set_style_text_font(l, font, 0);
  lv_obj_set_style_text_color(l, color, 0);
  lv_label_set_text(l, text);
  lv_obj_set_pos(l, x, y);
  return l;
}

lv_obj_t* uiTile(lv_obj_t* p, const char* txt, int x, int y, int w, int h, lv_event_cb_t cb, intptr_t id) {
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

void uiTilePaint(lv_obj_t* tile, bool on) {
  lv_obj_set_style_bg_color(tile, on ? COL_ACCENT : lv_color_hex(0x808080), 0);
  lv_obj_set_style_text_color(tile, on ? COL_BG : COL_TEXT, 0);
}

lv_obj_t* uiHamburger(lv_obj_t* header, lv_event_cb_t cb) {
  lv_obj_align(lv_obj_get_child(header, 0), LV_ALIGN_LEFT_MID, 46, 0);  // headerCreate adds the title first
  lv_obj_t* btn = lv_obj_create(header);
  lv_obj_remove_style_all(btn);
  lv_obj_set_size(btn, 40, HEADER_H);
  lv_obj_set_pos(btn, 0, 0);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
  for (int i = 0; i < 3; i++) {
    lv_obj_t* bar = lv_obj_create(btn);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, 20, 3);
    lv_obj_set_pos(bar, 10, 6 + i * 6);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(bar, COL_BG, 0);
  }
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
  return btn;
}

lv_obj_t* uiModalPanel() {
  lv_obj_t* p = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(p);
  lv_obj_set_size(p, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(p, COL_BG, 0);
  lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(p, COL_TEXT, 0);
  lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(p, LV_OBJ_FLAG_CLICKABLE);  // swallow touches
  return p;
}
