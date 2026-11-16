#include "matrix_screen.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "game_screen.h"
#include "../../net/time_service.h"
#include "../fonts/fonts.h"
#include "../ui_manager.h"

namespace {
constexpr int COLS = 45, SPACING = 7, STEP = 16, HOLD_MS = 2000, MEM_LOG_MS = 5000;

struct Drop {
  int16_t y;
  uint8_t speed, len;
  char txt[3][26];  // dim, mid, head; static buffers avoid heap churn on every frame
  lv_obj_t* lbl[3];
};

class MatrixScreen : public Screen {
 public:
  ~MatrixScreen() override {
    if (timer_) lv_timer_delete(timer_);
  }
  ScreenId id() const override { return ScreenId::Matrix; }

  void create(lv_obj_t* p) override {
    static const uint32_t col[3] = {0x004000, 0x00C000, 0xFFFFFF};
    static lv_style_t st[3];
    static bool inited;
    if (!inited) {
      inited = true;
      for (int k = 0; k < 3; k++) {
        lv_style_init(&st[k]);
        lv_style_set_text_font(&st[k], &font_14);
        lv_style_set_text_color(&st[k], lv_color_hex(col[k]));
        lv_style_set_text_line_space(&st[k], STEP - lv_font_get_line_height(&font_14));
      }
    }
    for (int i = 0; i < COLS; i++) {
      resetDrop(i);
      drops_[i].y = -(int)random(0, 240);
      for (int k = 0; k < 3; k++) {
        lv_obj_t* l = lv_label_create(p);
        lv_obj_add_style(l, &st[k], 0);
        lv_obj_set_x(l, i * SPACING);
        drops_[i].lbl[k] = l;
      }
    }
    // Hidden zone: a 2 s hold opens the game. A still finger produces no swipe gesture.
    lv_obj_t* z = lv_obj_create(p);
    lv_obj_remove_style_all(z);
    lv_obj_set_size(z, 80, 80);
    lv_obj_set_pos(z, 240, 0);
    lv_obj_add_event_cb(z, onPress, LV_EVENT_PRESSED, this);
    lv_obj_add_event_cb(z, onRelease, LV_EVENT_RELEASED, this);
    lv_obj_add_event_cb(z, onRelease, LV_EVENT_PRESS_LOST, this);

    clock_ = lv_label_create(p);  // opaque box masks the rain behind the digits
    lv_obj_set_style_text_font(clock_, &font_clock_48, 0);
    lv_obj_set_style_text_color(clock_, lv_color_hex(0x07E0), 0);
    lv_obj_set_style_bg_color(clock_, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(clock_, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(clock_, 3, 0);
    lv_label_set_text(clock_, "--:--:--");
    lv_obj_align(clock_, LV_ALIGN_TOP_MID, 0, 20);

    created_ = millis();
    step();
    timer_ = lv_timer_create([](lv_timer_t* t) { static_cast<MatrixScreen*>(lv_timer_get_user_data(t))->step(); }, 120, this);
  }

  void tick(uint32_t now) override {
    char buf[12] = "--:--:--";
    if (timeValid()) {
      struct tm tm;
      localTimeOf(time(nullptr), tm);
      snprintf(buf, sizeof buf, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
    }
    if (strcmp(buf, lv_label_get_text(clock_)) != 0) lv_label_set_text(clock_, buf);
    if (pressAt_ && now - pressAt_ >= HOLD_MS) {
      pressAt_ = 0;
      uiOpenModal(createGameScreen());
    }
    if (!memLogged_ && now - created_ >= MEM_LOG_MS) {
      memLogged_ = true;
      lv_mem_monitor_t m;
      lv_mem_monitor(&m);
      Serial.printf("[MEM] matrix lvfree=%u\n", (unsigned)m.free_size);
    }
  }

 private:
  Drop drops_[COLS];
  lv_obj_t* clock_ = nullptr;
  lv_timer_t* timer_ = nullptr;
  uint32_t pressAt_ = 0, created_ = 0;
  bool memLogged_ = false, paused_ = false;

  static void onPress(lv_event_t* e) { static_cast<MatrixScreen*>(lv_event_get_user_data(e))->pressAt_ = millis(); }
  static void onRelease(lv_event_t* e) { static_cast<MatrixScreen*>(lv_event_get_user_data(e))->pressAt_ = 0; }

  void resetDrop(int i) {
    drops_[i].y = 0;
    drops_[i].speed = (uint8_t)random(5, 15);
    drops_[i].len = (uint8_t)random(4, 17);
  }

  void step() {
    if (uiModalActive()) {  // pause and hide the rain while the game is open
      if (!paused_) {
        paused_ = true;
        for (Drop& d : drops_)
          for (lv_obj_t* l : d.lbl) lv_obj_add_flag(l, LV_OBJ_FLAG_HIDDEN);
      }
      return;
    }
    paused_ = false;
    for (int i = 0; i < COLS; i++) {
      Drop& d = drops_[i];
      d.y += d.speed;
      if (d.y > 240) resetDrop(i);
      const int n[3] = {d.len - 4, 3, 1};  // dim, mid, head (len >= 4)
      int off = 0;
      for (int k = 0; k < 3; k++) {
        char* t = d.txt[k];
        int w = 0;
        for (int j = 0; j < n[k]; j++) {
          t[w++] = (char)random(33, 127);
          if (j < n[k] - 1) t[w++] = '\n';
        }
        t[w] = 0;
        lv_obj_t* l = d.lbl[k];
        if (lv_obj_has_flag(l, LV_OBJ_FLAG_HIDDEN) != (n[k] == 0)) lv_obj_set_flag(l, LV_OBJ_FLAG_HIDDEN, n[k] == 0);
        if (n[k]) {
          lv_label_set_text_static(l, t);
          if (lv_obj_get_y(l) != d.y + off * STEP) lv_obj_set_y(l, d.y + off * STEP);
        }
        off += n[k];
      }
    }
  }
};
}  // namespace

Screen* createMatrixScreen() { return new MatrixScreen(); }
