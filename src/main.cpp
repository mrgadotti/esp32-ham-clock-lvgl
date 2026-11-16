#include <Arduino.h>
#include <lvgl.h>

#include "board.h"
#include "hal/display.h"

static void bar(int x, uint32_t color, const char* text, bool darkText) {
  lv_obj_t* o = lv_obj_create(lv_screen_active());
  lv_obj_remove_style_all(o);
  lv_obj_set_size(o, 70, 200);
  lv_obj_set_pos(o, 10 + x * 78, 20);
  lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(o, lv_color_hex(color), 0);
  lv_obj_t* l = lv_label_create(o);
  lv_label_set_text(l, text);
  lv_obj_set_style_text_color(l, darkText ? lv_color_black() : lv_color_white(), 0);
  lv_obj_center(l);
}

void setup() {
  Serial.begin(115200);
  displayInit(1, DISPLAY_INVERT_DEFAULT);
  lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);
  bar(0, 0xFF0000, "RED", false);
  bar(1, 0x00FF00, "GREEN", true);
  bar(2, 0x0000FF, "BLUE", false);
  bar(3, 0xFFFFFF, "WHITE", true);
}

void loop() {
  lv_timer_handler();
  delay(5);
}
