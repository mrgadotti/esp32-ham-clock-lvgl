#include "ui_kit.h"

#include "../fonts/fonts.h"
#include "../theme.h"
#include "header.h"

lv_obj_t* uiLabel(lv_obj_t* parent, const lv_font_t* font, lv_color_t color, const char* text, int x, int y, int w,
                  lv_text_align_t align) {
  lv_obj_t* l = lv_label_create(parent);
  lv_obj_set_style_text_font(l, font, 0);
  lv_obj_set_style_text_color(l, color, 0);
  lv_label_set_text(l, text);
  if (w) lv_obj_set_width(l, w);
  if (align != LV_TEXT_ALIGN_AUTO) lv_obj_set_style_text_align(l, align, 0);
  lv_obj_set_pos(l, x, y);
  return l;
}

void uiPressFx(lv_obj_t* obj, bool /*solid*/) {
  static lv_style_t press;
  static bool init = false;
  if (!init) {
    init = true;
    lv_style_init(&press);
    lv_style_set_bg_color(&press, COL_ACCENT_DARK);  // a PRESSED style outranks the local default-state bg set by uiTilePaint
    lv_style_set_bg_opa(&press, LV_OPA_COVER);
  }
  lv_obj_add_style(obj, &press, LV_STATE_PRESSED);
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
  uiPressFx(t, true);
  return t;
}

void uiTilePaint(lv_obj_t* tile, bool on) {
  lv_obj_set_style_bg_color(tile, on ? COL_ACCENT : COL_DIM, 0);
  lv_obj_set_style_text_color(tile, on ? COL_BG : COL_TEXT, 0);
}

lv_obj_t* uiHamburger(lv_obj_t* header, lv_event_cb_t cb) {
  lv_obj_align(headerTitle(header), LV_ALIGN_LEFT_MID, 46, 0);  // headerCreate adds the title first
  lv_obj_t* btn = lv_obj_create(header);
  lv_obj_remove_style_all(btn);
  lv_obj_set_size(btn, 50, HEADER_H);
  lv_obj_set_pos(btn, 0, 0);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
  for (int i = 0; i < 3; i++) {
    lv_obj_t* bar = lv_obj_create(btn);
    lv_obj_remove_style_all(bar);
    lv_obj_remove_flag(bar, LV_OBJ_FLAG_CLICKABLE);  // else the bars swallow taps on the icon itself
    lv_obj_set_size(bar, 20, 3);
    lv_obj_set_pos(bar, 10, 6 + i * 6);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(bar, COL_BG, 0);
  }
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
  uiPressFx(btn);
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

lv_obj_t* uiScrollArea(lv_obj_t* parent, int x, int y, int w, int h) {
  lv_obj_t* c = lv_obj_create(parent);
  lv_obj_remove_style_all(c);
  lv_obj_set_pos(c, x, y);
  lv_obj_set_size(c, w, h);
  lv_obj_set_scroll_dir(c, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(c, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_style_width(c, 3, LV_PART_SCROLLBAR);
  lv_obj_set_style_bg_opa(c, LV_OPA_COVER, LV_PART_SCROLLBAR);
  lv_obj_set_style_bg_color(c, COL_DIM, LV_PART_SCROLLBAR);
  return c;
}
