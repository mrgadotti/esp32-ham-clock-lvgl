#include <Arduino.h>
#include <lvgl.h>

#include "board.h"
#include "hal/display.h"
#include "hal/touch.h"
#include "hal/touch_cal_ui.h"

static lv_obj_t* dot;

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

  touchInit();
  touchCreateIndev();
  if (!touchCalLoadSaved() || touchHeldAtBoot(2000)) touchCalRun(nullptr);

  // Touch test: corner markers and a dot that follows the last touched point.
  static const int16_t corners[4][2] = {{10, 10}, {309, 10}, {10, 229}, {309, 229}};
  for (auto& c : corners) {
    lv_obj_t* m = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(m);
    lv_obj_set_size(m, 5, 5);
    lv_obj_set_pos(m, c[0] - 2, c[1] - 2);
    lv_obj_set_style_bg_opa(m, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(m, lv_palette_main(LV_PALETTE_YELLOW), 0);
  }
  dot = lv_obj_create(lv_screen_active());
  lv_obj_remove_style_all(dot);
  lv_obj_set_size(dot, 10, 10);
  lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(dot, lv_palette_main(LV_PALETTE_PURPLE), 0);
  lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
}

void loop() {
  lv_point_t p;
  lv_indev_t* in = lv_indev_get_next(nullptr);
  if (in && lv_indev_get_state(in) == LV_INDEV_STATE_PRESSED) {
    lv_indev_get_point(in, &p);
    lv_obj_set_pos(dot, p.x - 5, p.y - 5);
    lv_obj_remove_flag(dot, LV_OBJ_FLAG_HIDDEN);
  }
  lv_timer_handler();
  delay(5);
}
