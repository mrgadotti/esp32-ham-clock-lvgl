#include "touch_cal_ui.h"

#include <Preferences.h>
#include <lvgl.h>
#include <math.h>

#include "touch.h"

static const int16_t TARGETS[3][2] = {{32, 24}, {288, 120}, {160, 216}};
static const int16_t VERIFY[2] = {160, 120};

bool touchCalLoadSaved() {
  Preferences p;
  TouchCal c;
  bool ok = false;
  if (p.begin("hamclk", true)) {
    ok = p.getBytesLength("tcal") == sizeof(c) && p.getBytes("tcal", &c, sizeof(c)) == sizeof(c);
    p.end();
  }
  if (ok) touchSetCal(c);
  return ok;
}

static lv_obj_t* scrn;
static lv_obj_t* msg;
static lv_obj_t* cross[2];  // horizontal + vertical bar

static void showTarget(int16_t x, int16_t y, const char* text) {
  lv_obj_set_pos(cross[0], x - 10, y - 1);
  lv_obj_set_pos(cross[1], x - 1, y - 10);
  lv_label_set_text(msg, text);
}

static lv_obj_t* bar(int w, int h) {
  lv_obj_t* o = lv_obj_create(scrn);
  lv_obj_remove_style_all(o);
  lv_obj_set_size(o, w, h);
  lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(o, lv_color_white(), 0);
  return o;
}

static void pump(uint32_t ms) {
  for (uint32_t t0 = millis(); millis() - t0 < ms;) { lv_timer_handler(); delay(10); }
}

static void waitRelease() {
  int16_t x, y;
  pump(50);
  while (touchReadRaw(x, y)) pump(10);
}

static void waitTap(int16_t& rx, int16_t& ry) {
  while (!touchReadRaw(rx, ry)) pump(10);
  pump(30);
  touchReadRaw(rx, ry);  // settled reading; keeps the first one if the finger just lifted
}

void touchCalRun(void (*onDone)(bool saved)) {
  lv_obj_t* prev = lv_screen_active();
  scrn = lv_obj_create(nullptr);
  lv_obj_set_style_bg_color(scrn, lv_color_black(), 0);
  cross[0] = bar(21, 3);
  cross[1] = bar(3, 21);
  msg = lv_label_create(scrn);
  lv_obj_set_style_text_color(msg, lv_color_white(), 0);
  lv_obj_align(msg, LV_ALIGN_CENTER, 0, -40);
  lv_screen_load(scrn);

  touchSetCal(touchCalDefault());
  TouchCal c;
  for (;;) {
    int16_t raw[3][2];
    for (int i = 0; i < 3; i++) {
      showTarget(TARGETS[i][0], TARGETS[i][1], "Touch the cross");
      waitTap(raw[i][0], raw[i][1]);
      waitRelease();
    }
    if (!touchCalSolve(raw, TARGETS, c)) { showTarget(-100, -100, "Bad points, retry"); pump(1500); continue; }
    showTarget(VERIFY[0], VERIFY[1], "Verify: touch the cross");
    int16_t rx, ry, x, y;
    waitTap(rx, ry);
    waitRelease();
    touchCalApply(c, rx, ry, x, y);
    if (hypotf(x - VERIFY[0], y - VERIFY[1]) <= 8) break;
    showTarget(-100, -100, "Too inaccurate, retry");
    pump(1500);
  }
  touchSetCal(c);
  Preferences p;
  bool saved = p.begin("hamclk", false) && p.putBytes("tcal", &c, sizeof(c)) == sizeof(c);
  p.end();
  lv_screen_load(prev);
  lv_obj_delete(scrn);
  if (onDone) onDone(saved);
}
