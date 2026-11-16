#include "game_screen.h"

#include <Arduino.h>
#include <initializer_list>
#include <math.h>

#include "i18n.h"
#include "../fonts/fonts.h"
#include "../ui_manager.h"

namespace {
constexpr int CX = 160, CY = 120, R_OUT = 115, R_IN = 60, HIT_PX = 14;
constexpr float BASE_DEG_S = 46.0f;
constexpr uint32_t SECOND_DELAY = 60000, GREEN_LIFE = 5000;

struct Tgt {
  bool on = false, outer = true;
  float ang = 0;
  uint32_t shown = 0;
};

class GameScreen : public Screen {
 public:
  ~GameScreen() override {
    if (timer_) lv_timer_delete(timer_);
  }
  ScreenId id() const override { return ScreenId::Game; }
  bool blocksSwipe() const override { return running_; }
  void tick(uint32_t) override {}

  void create(lv_obj_t* p) override {
    lv_obj_t* cv = lv_obj_create(p);
    lv_obj_remove_style_all(cv);
    lv_obj_set_size(cv, 320, 240);
    lv_obj_remove_flag(cv, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(cv, onDraw, LV_EVENT_DRAW_MAIN, this);
    cv_ = cv;

    startLbl_ = button(p, 0, 0, tr(Tr::GameStart), onStart, this);
    button(p, 264, 190, tr(Tr::GamePtt), onPtt, this, LV_EVENT_PRESSED);
    button(p, 0, 190, tr(Tr::GameExit), [](lv_event_t*) { uiCloseModal(); }, nullptr);

    score_ = text(p, &font_18, 4, 56, lv_color_hex(0x00FF00));
    timeLbl_ = text(p, &font_18, 236, 6, lv_color_white());
    lv_obj_set_width(timeLbl_, 80);
    lv_obj_set_style_text_align(timeLbl_, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_text(text(p, &font_14, 276, 92, lv_color_white()), tr(Tr::GameBand));
    lv_label_set_text(text(p, &font_14, 288, 108, lv_color_hex(0xFFFF00)), "2m");
    lv_label_set_text(text(p, &font_14, 282, 124, lv_color_hex(0x4040FF)), "70cm");

    intro_ = lv_obj_create(p);
    lv_obj_remove_style_all(intro_);
    lv_obj_set_size(intro_, 240, 92);
    lv_obj_set_pos(intro_, 40, 72);
    lv_obj_set_style_bg_color(intro_, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(intro_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(intro_, 1, 0);
    lv_obj_set_style_border_color(intro_, lv_color_hex(0x808080), 0);
    lv_label_set_text(text(intro_, &font_18, 8, 4, lv_color_hex(0xFFFF00)), tr(Tr::GameTitle));
    lv_obj_t* h = text(intro_, &font_14, 8, 32, lv_color_white());
    lv_label_set_text_fmt(h, "%s\n%s\n%s\n%s", tr(Tr::GameHint1), tr(Tr::GameHint2), tr(Tr::GameHint3), tr(Tr::GameUnlis));

    over_ = lv_obj_create(p);
    lv_obj_remove_style_all(over_);
    lv_obj_set_size(over_, 200, 48);
    lv_obj_set_pos(over_, 60, 96);
    lv_obj_set_style_bg_color(over_, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(over_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(over_, 1, 0);
    lv_obj_set_style_border_color(over_, lv_color_hex(0xFF0000), 0);
    lv_obj_t* ol = text(over_, &font_18, 0, 0, lv_color_hex(0xFF0000));
    lv_label_set_text(ol, tr(Tr::GameOver));
    lv_obj_center(ol);
    lv_obj_add_flag(over_, LV_OBJ_FLAG_HIDDEN);

    timer_ = lv_timer_create([](lv_timer_t* t) { static_cast<GameScreen*>(lv_timer_get_user_data(t))->frame(); }, 40, this);
    refreshHud(true);
  }

 private:
  lv_obj_t *cv_ = nullptr, *startLbl_ = nullptr, *score_ = nullptr, *timeLbl_ = nullptr, *intro_ = nullptr, *over_ = nullptr;
  bool dirty_ = true;
  lv_timer_t* timer_ = nullptr;
  bool running_ = false, over_flag_ = false;
  int caught_ = 0, missed_ = 0, shownCaught_ = -1, shownSec_ = -1;
  uint32_t start_ = 0, last_ = 0, lastPtt_ = 0, nextSpawn_ = 0, next2_ = 0, nextGreen_ = 0, frozenSec_ = 0;
  float scan_ = 0;
  Tgt t1_, t2_, g_;

  static lv_obj_t* text(lv_obj_t* p, const lv_font_t* f, int x, int y, lv_color_t c) {
    lv_obj_t* l = lv_label_create(p);
    lv_obj_set_style_text_font(l, f, 0);
    lv_obj_set_style_text_color(l, c, 0);
    lv_obj_set_pos(l, x, y);
    return l;
  }
  static lv_obj_t* button(lv_obj_t* p, int x, int y, const char* s, lv_event_cb_t cb, void* ud, lv_event_code_t ev = LV_EVENT_CLICKED) {
    lv_obj_t* b = lv_button_create(p);
    lv_obj_set_size(b, 56, 50);
    lv_obj_set_pos(b, x, y);
    lv_obj_set_ext_click_area(b, 22);
    lv_obj_set_style_bg_color(b, lv_color_black(), 0);
    lv_obj_set_style_border_width(b, 1, 0);
    lv_obj_set_style_border_color(b, lv_color_hex(0xC0C0C0), 0);
    lv_obj_set_style_radius(b, 10, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_add_event_cb(b, cb, ev, ud);
    lv_obj_t* l = lv_label_create(b);
    lv_obj_set_style_text_font(l, &font_14, 0);
    lv_label_set_text(l, s);
    lv_obj_center(l);
    return l;
  }
  static void onStart(lv_event_t* e) { static_cast<GameScreen*>(lv_event_get_user_data(e))->startStop(); }
  static void onPtt(lv_event_t* e) { static_cast<GameScreen*>(lv_event_get_user_data(e))->ptt(millis()); }

  float degPerSec(uint32_t now) const {
    uint32_t el = (running_ || over_flag_) ? now - start_ : 0;
    return BASE_DEG_S * (1.0f + caught_ * (el >= 60000 ? 0.03f : 0.05f));
  }
  uint32_t cooldownMs(uint32_t now) const { return (uint32_t)(360.0f / degPerSec(now) * 250.0f + 0.5f); }
  uint32_t lifeMs(uint32_t now) const { return (uint32_t)(360.0f / degPerSec(now) * 2500.0f + 0.5f); }
  void resetPtt(uint32_t now) { lastPtt_ = now - cooldownMs(now); }

  static void pos(const Tgt& t, int& x, int& y) {
    float a = t.ang * (float)M_PI / 180.0f;
    int r = t.outer ? R_OUT : R_IN;
    x = CX + (int)(sinf(a) * r);
    y = CY - (int)(cosf(a) * r);
  }
  static bool inButton(int x, int y) {  // the three 100x100 touch zones
    return (x < 100 && (y < 100 || y >= 140)) || (x >= 220 && y >= 140);
  }
  static void randomSpot(Tgt& t, uint32_t now) {
    for (int i = 0; i < 24; i++) {
      t.outer = random(0, 2) == 0;
      t.ang = (float)random(0, 360);
      int x, y;
      pos(t, x, y);
      if (!inButton(x, y)) break;
    }
    t.on = true;
    t.shown = now;
  }
  bool hits(const Tgt& t) const {
    float a = (t.outer ? scan_ : fmodf(scan_ + 180.0f, 360.0f)) * (float)M_PI / 180.0f;
    int r = t.outer ? R_OUT : R_IN, ax = (int)(sinf(a) * r), ay = -(int)(cosf(a) * r), x, y;
    pos(t, x, y);
    int dx = CX + ax - x, dy = CY + ay - y;
    return dx * dx + dy * dy <= HIT_PX * HIT_PX;
  }

  void startStop() {
    uint32_t now = millis();
    if (running_) {  // STOP: back to intro
      running_ = over_flag_ = false;
      t1_.on = t2_.on = g_.on = false;
    } else {
      running_ = true;
      over_flag_ = false;
      caught_ = missed_ = 0;
      frozenSec_ = 0;
      start_ = last_ = now;
      scan_ = 0;
      t1_.on = t2_.on = g_.on = false;
      resetPtt(now);
      nextSpawn_ = now + random(200, 700);
      next2_ = now + SECOND_DELAY + random(500, 3000);
      nextGreen_ = now + random(9000, 18001);
    }
    refreshHud(true);
  }

  void ptt(uint32_t now) {
    if (!running_ || now - lastPtt_ < cooldownMs(now)) return;
    lastPtt_ = now;
    if (g_.on && hits(g_)) {
      if (caught_ > 0) caught_--;
      g_.on = false;
      nextGreen_ = now + random(9000, 18001);
      return;
    }
    bool hit = false;
    if (t1_.on && hits(t1_)) { caught_++; t1_.on = false; hit = true; }
    if (t2_.on && hits(t2_)) {
      caught_++;
      t2_.on = false;
      next2_ = now + random(3500, 9501);
      hit = true;
    }
    if (hit) {
      resetPtt(now);
      if (!t1_.on && !t2_.on) nextSpawn_ = now + random(450, 1201);
    }
  }

  // A red target expired without being hit.
  void miss(Tgt& t, uint32_t now) {
    t.on = false;
    resetPtt(now);
    if (++missed_ >= 3) {
      frozenSec_ = (now - start_) / 1000;
      over_flag_ = true;
      running_ = false;
      t1_.on = t2_.on = g_.on = false;
    }
  }

  void frame() {
    uint32_t now = millis();
    if (running_) {
      float dt = (now - last_) / 1000.0f;
      last_ = now;
      scan_ = fmodf(scan_ + degPerSec(now) * dt, 360.0f);
      if (!t1_.on && !t2_.on && now >= nextSpawn_) {
        randomSpot(t1_, now);
        resetPtt(now);
      }
      if (now - start_ >= SECOND_DELAY && !t2_.on && now >= next2_) {
        randomSpot(t2_, now);
        resetPtt(now);
      }
      if (!g_.on && now >= nextGreen_) {
        randomSpot(g_, now);
        nextGreen_ = now + random(9000, 18001);
      }
      if (t1_.on && now - t1_.shown >= lifeMs(now)) {
        miss(t1_, now);
        if (running_ && !t2_.on) nextSpawn_ = now + random(450, 1201);
      }
      if (running_ && t2_.on && now - t2_.shown >= lifeMs(now)) {
        next2_ = now + random(3500, 9501);
        miss(t2_, now);
        if (running_ && !t1_.on) nextSpawn_ = now + random(450, 1201);
      }
      if (running_ && g_.on && now - g_.shown >= GREEN_LIFE) g_.on = false;
      if (over_flag_) refreshHud(true);
    }
    refreshHud(false);
    if (running_ || dirty_) {  // only the radar disc and the miss markers change
      dirty_ = false;
      lv_area_t a = {CX - R_OUT - 8, 0, CX + R_OUT + 8, 239}, m = {2, 90, 24, 150};
      lv_obj_invalidate_area(cv_, &a);
      lv_obj_invalidate_area(cv_, &m);
    }
  }

  void refreshHud(bool force) {
    if (force) dirty_ = true;
    int sec = running_ ? (millis() - start_) / 1000 : over_flag_ ? frozenSec_ : 0;
    if (force) {
      lv_label_set_text(startLbl_, tr(running_ ? Tr::GameStop : Tr::GameStart));
      lv_obj_set_flag(intro_, LV_OBJ_FLAG_HIDDEN, running_ || over_flag_);
      lv_obj_set_flag(over_, LV_OBJ_FLAG_HIDDEN, !over_flag_);
    }
    if (force || caught_ != shownCaught_) {
      shownCaught_ = caught_;
      lv_label_set_text_fmt(score_, "%03d", caught_);
    }
    if (force || sec != shownSec_) {
      shownSec_ = sec;
      lv_label_set_text_fmt(timeLbl_, "%ds", sec);
    }
  }

  static void dot(lv_layer_t* l, int x, int y, int r, lv_color_t c) {
    lv_draw_arc_dsc_t d;
    lv_draw_arc_dsc_init(&d);
    d.color = c;
    d.width = r;  // width == radius fills the disc
    d.radius = r;
    d.center = {x, y};
    d.start_angle = 0;
    d.end_angle = 360;
    lv_draw_arc(l, &d);
  }
  static void ring(lv_layer_t* l, int r, lv_color_t c) {
    lv_draw_arc_dsc_t d;
    lv_draw_arc_dsc_init(&d);
    d.color = c;
    d.width = 1;
    d.radius = r;
    d.center = {CX, CY};
    d.start_angle = 0;
    d.end_angle = 360;
    lv_draw_arc(l, &d);
  }
  static void line(lv_layer_t* l, int x2, int y2, lv_color_t c) {
    lv_draw_line_dsc_t d;
    lv_draw_line_dsc_init(&d);
    d.color = c;
    d.width = 1;
    d.p1 = {CX, CY};
    d.p2 = {x2, y2};
    lv_draw_line(l, &d);
  }

  static void onDraw(lv_event_t* e) {
    GameScreen* g = static_cast<GameScreen*>(lv_event_get_user_data(e));
    lv_layer_t* l = lv_event_get_layer(e);
    ring(l, R_OUT, lv_color_hex(0xFFFF00));
    ring(l, R_IN, lv_color_hex(0x0000FF));
    dot(l, CX, CY, 3, lv_color_hex(0x00FF00));
    for (int i = 0; i < 3; i++)
      if (g->missed_ > i) dot(l, 12, 102 + i * 18, 5, lv_color_hex(0xFF0000));
      else ring2(l, 12, 102 + i * 18);
    int x, y;
    for (const Tgt* t : {&g->t1_, &g->t2_})
      if (t->on) { pos(*t, x, y); dot(l, x, y, t->outer ? 6 : 4, lv_color_hex(0xFF0000)); }
    if (g->g_.on) { pos(g->g_, x, y); dot(l, x, y, 4, lv_color_hex(0x00FF00)); }
    if (g->running_ || g->over_flag_) {
      float a = g->scan_ * (float)M_PI / 180.0f;
      line(l, CX + (int)(sinf(a) * R_OUT), CY - (int)(cosf(a) * R_OUT), lv_color_hex(0xFFFF00));
      line(l, CX - (int)(sinf(a) * R_IN), CY + (int)(cosf(a) * R_IN), lv_color_hex(0x0000FF));
    }
  }
  static void ring2(lv_layer_t* l, int x, int y) {  // empty miss marker: dark grey outline
    lv_draw_arc_dsc_t d;
    lv_draw_arc_dsc_init(&d);
    d.color = lv_color_hex(0x606060);
    d.width = 1;
    d.radius = 5;
    d.center = {x, y};
    d.start_angle = 0;
    d.end_angle = 360;
    lv_draw_arc(l, &d);
  }

};
}  // namespace

Screen* createGameScreen() { return new GameScreen(); }
