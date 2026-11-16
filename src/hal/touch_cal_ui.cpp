#include "touch_cal_ui.h"

#include <lvgl.h>
#include <math.h>
#include <string.h>

#include "app/config_store.h"
#include "touch.h"

static_assert(sizeof(TouchCal) == sizeof(Config::tcal), "tcal blob layout");

static const int16_t TARGETS[3][2] = {{32, 24}, {288, 120}, {160, 216}};
static const int16_t VERIFY[2] = {160, 120};

bool touchCalLoadSaved() {
  Config cf;
  cfgGet(cf);
  TouchCal c;
  memcpy(&c, cf.tcal, sizeof c);
  bool ok = c.a != 0 || c.b != 0;  // all zero = never calibrated
  const float v[6] = {c.a, c.b, c.c, c.d, c.e, c.f};
  for (float f : v) ok = ok && isfinite(f);
  if (ok) touchSetCal(c);
  return ok;
}

enum Phase : uint8_t { WAIT_PRESS, WAIT_RELEASE, SHOW_MSG };

static struct {
  lv_obj_t *scrn, *prev, *msg, *cross[2];
  lv_timer_t* timer;
  void (*onDone)(bool);
  TouchCal cal;
  int16_t raw[3][2];
  int16_t sample[2];
  uint8_t step;      // 0..2 targets, 3 verify
  Phase phase;
  uint8_t ticks, misses;
  bool haveSample, mirror;
} S;

static void showTarget(int16_t x, int16_t y, const char* text) {
  lv_obj_set_pos(S.cross[0], x - 10, y - 1);
  lv_obj_set_pos(S.cross[1], x - 1, y - 10);
  lv_label_set_text(S.msg, text);
}

static void showStep() {
  if (S.step < 3) showTarget(TARGETS[S.step][0], TARGETS[S.step][1], "Touch the cross");
  else showTarget(VERIFY[0], VERIFY[1], "Verify: touch the cross");
}

static void showMsg(const char* text) {
  showTarget(-100, -100, text);
  S.phase = SHOW_MSG;
  S.ticks = 0;
}

static lv_obj_t* bar(int w, int h) {
  lv_obj_t* o = lv_obj_create(S.scrn);
  lv_obj_remove_style_all(o);
  lv_obj_set_size(o, w, h);
  lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(o, lv_color_white(), 0);
  return o;
}

static void finish(bool saved) {
  lv_timer_delete(S.timer);
  if (lv_obj_is_valid(S.prev)) lv_screen_load(S.prev);
  lv_obj_delete(S.scrn);
  S.timer = nullptr;
  touchSetMirror(S.mirror);
  touchEnable(true);
  if (S.onDone) S.onDone(saved);
}

// A completed tap (press then release) yielded S.sample; advance the state machine.
static void onTap() {
  if (S.step < 3) {
    S.raw[S.step][0] = S.sample[0];
    S.raw[S.step][1] = S.sample[1];
    S.step++;
    if (S.step == 3) {
      if (!touchCalSolve(S.raw, TARGETS, S.cal)) { S.step = 0; showMsg("Bad points, retry"); return; }
      touchSetCal(S.cal);
    }
    showStep();
    return;
  }
  int16_t x, y;
  touchCalApply(S.cal, S.sample[0], S.sample[1], x, y);
  if (hypotf(x - VERIFY[0], y - VERIFY[1]) > 8) { S.step = 0; showMsg("Too inaccurate, retry"); return; }
  static TouchCal toSave;
  toSave = S.cal;
  finish(cfgUpdate([](Config& cf) { memcpy(cf.tcal, &toSave, sizeof toSave); }));
}

static void tick(lv_timer_t*) {
  int16_t rx, ry;
  bool pressed = touchReadRaw(rx, ry);
  switch (S.phase) {
    case SHOW_MSG:
      if (++S.ticks >= 75) { S.phase = WAIT_PRESS; showStep(); }  // 1.5 s at 20 ms
      break;
    case WAIT_PRESS:
      if (pressed) { S.phase = WAIT_RELEASE; S.ticks = 0; S.misses = 0; S.haveSample = false; }
      break;
    case WAIT_RELEASE:
      if (pressed) {
        S.misses = 0;
        if (++S.ticks <= 3) { S.sample[0] = rx; S.sample[1] = ry; S.haveSample = true; }  // settle on the 3rd read; short taps keep the last
      } else if (++S.misses >= 2) {  // debounce pressure dips
        S.phase = WAIT_PRESS;
        if (S.haveSample) onTap();
      }
      break;
  }
}

void touchCalRun(void (*onDone)(bool saved)) {
  if (S.timer) return;  // already running
  S = {};
  S.onDone = onDone;
  S.prev = lv_screen_active();
  S.scrn = lv_obj_create(nullptr);
  lv_obj_set_style_bg_color(S.scrn, lv_color_black(), 0);
  S.cross[0] = bar(21, 3);
  S.cross[1] = bar(3, 21);
  S.msg = lv_label_create(S.scrn);
  lv_obj_set_style_text_color(S.msg, lv_color_white(), 0);
  lv_obj_align(S.msg, LV_ALIGN_CENTER, 0, -40);
  showStep();
  lv_screen_load(S.scrn);

  // The stored cal must always be the un-mirrored mapping (readCb flips after applying it), so
  // mirror is off while calibrating and restored in finish().
  S.mirror = touchGetMirror();
  touchSetMirror(false);
  touchSetCal(touchCalDefault());
  touchEnable(false);  // no clicks/gestures reach the UI underneath
  S.timer = lv_timer_create(tick, 20, nullptr);
}
