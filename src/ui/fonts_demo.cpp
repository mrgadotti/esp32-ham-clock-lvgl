#include "fonts_demo.h"
#include "fonts/fonts.h"
#include "theme.h"
#include "widgets/header.h"

void fontsDemoScreen() {
  lv_obj_t* scr = lv_screen_active();
  lv_obj_t* h = headerCreate(scr, Tr::Forecast);
  headerSetClock(h, "14:05Z");
  static const lv_font_t* fonts[] = {&font_14, &font_18, &font_24};
  int y = HEADER_H + 4;
  for (const lv_font_t* f : fonts) {
    lv_obj_t* l = lv_label_create(scr);
    lv_obj_set_style_text_font(l, f, 0);
    lv_label_set_text(l, "Previsão Pressão Umidade ÁÉÍÓÚ ãõç 14:05");
    lv_obj_set_pos(l, 4, y);
    y += lv_font_get_line_height(f) + 4;
  }
  lv_obj_t* c = lv_label_create(scr);
  lv_obj_set_style_text_font(c, &font_clock_48, 0);
  lv_obj_set_style_text_color(c, COL_FREQ, 0);
  lv_label_set_text(c, "12:34:56");
  lv_obj_set_pos(c, 4, y);
}
