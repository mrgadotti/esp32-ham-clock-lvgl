#include "theme.h"
#include "fonts/fonts.h"
#include "src/themes/lv_theme_private.h"  // lv_theme_t is opaque in the public API

static lv_style_t s_screen;
static lv_theme_t s_theme;

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
  s_theme = *base;
  lv_theme_set_parent(&s_theme, base);
  lv_theme_set_apply_cb(&s_theme, applyCb);
  lv_display_set_theme(disp, &s_theme);
}
