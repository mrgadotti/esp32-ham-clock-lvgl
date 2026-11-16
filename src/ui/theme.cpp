#include "theme.h"
#include "fonts/fonts.h"

static lv_style_t s_screen;

static void applyCb(lv_theme_t*, lv_obj_t* obj) {
  if (lv_obj_get_parent(obj) == nullptr) lv_obj_add_style(obj, &s_screen, 0);  // screens only
}

void themeApply(lv_display_t* disp) {
  lv_theme_t* base = lv_theme_default_init(disp, COL_ACCENT, COL_MODE, true, &font_14);
  lv_style_init(&s_screen);
  lv_style_set_bg_color(&s_screen, COL_BG);
  lv_style_set_bg_opa(&s_screen, LV_OPA_COVER);
  lv_style_set_text_color(&s_screen, COL_TEXT);
  lv_style_set_text_font(&s_screen, &font_14);
  // The simple theme is only a carrier: parent = default theme, apply_cb = ours (runs after the parent's).
  lv_theme_t* top = lv_theme_simple_init(disp);
  lv_theme_set_parent(top, base);
  lv_theme_set_apply_cb(top, applyCb);
  lv_display_set_theme(disp, top);
}
