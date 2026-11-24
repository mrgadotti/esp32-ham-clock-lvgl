#include "aprs_radar_screen.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "aprs_symbol.h"
#include "geo.h"
#include "i18n.h"
#include "radar_label.h"
#include "radar_proj.h"
#include "../../app/config_store.h"
#include "../../app/data_store.h"
#include "../../app/service_coords.h"
#include "../fonts/fonts.h"
#include "../theme.h"
#include "../widgets/aprs_icon.h"
#include "../widgets/header.h"
#include "../widgets/ui_kit.h"

namespace {
constexpr int R_PX = 100, OX = 160, OY = HEADER_H + (SCREEN_H - HEADER_H) / 2, MAXN = kAprsStations;
// Zoom button column (right edge): + and - above the reset button
constexpr int kBtnX = 272, kBtnW = 44, kBtnH = 28, kBtnY0 = 144, kBtnYStep = 32;

// x/y/img/lab are the screen layout, rebuilt by layout() whenever the view or the data changes (not per render strip)
struct Pt {
  float distKm, bearing; lv_color_t col; char call[10]; AprsIcon icon;
  bool vis; int16_t x, y, lab;  // lab: index into labs_, -1 = no label
  const lv_image_dsc_t* img;
  lv_area_t box;  // icon (or dot) bounds
};
struct Lab { lv_area_t a; char s[16]; lv_color_t c; };

bool hit(const lv_area_t& k, const lv_area_t& a) { return a.x1 <= k.x2 && k.x1 <= a.x2 && a.y1 <= k.y2 && k.y1 <= a.y2; }

lv_color_t stationColor(const AprsStation& s) {  // as getAprsRadarColorForStation (dark theme)
  if (s.sym[0] == '/') {
    if (s.sym[1] == '>') return lv_color_hex(0xFF0000);          // car
    if (s.sym[1] == '-') return lv_color_hex(0xFFFF00);          // house
    if (s.sym[1] == '[') return lv_color_make(0x38, 0x80, 0xFF);  // human (0x3C1F)
    if (s.sym[1] == 'V') return lv_color_hex(0xFFA500);          // HamClock
  }
  return lv_color_hex(0x00FF00);
}

class AprsRadarScreen : public Screen {
 public:
  ScreenId id() const override { return ScreenId::AprsRadar; }

  void create(lv_obj_t* p) override {
    hdr_ = headerCreate(p, Tr::AprsRadar);
    cv_ = lv_obj_create(p);
    lv_obj_remove_style_all(cv_);
    lv_obj_set_size(cv_, SCREEN_W, SCREEN_H - HEADER_H);
    lv_obj_set_pos(cv_, 0, HEADER_H);
    lv_obj_add_flag(cv_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(cv_, onDraw, LV_EVENT_DRAW_MAIN, this);
    lv_obj_add_event_cb(cv_, onTap, LV_EVENT_SHORT_CLICKED, this);
    lv_obj_add_event_cb(cv_, onPan, LV_EVENT_ALL, this);

    zoomButton(p, kBtnY0, "+", onZoomIn);
    zoomButton(p, kBtnY0 + kBtnYStep, "-", onZoomOut);
    lv_obj_t* b = zoomButton(p, kBtnY0 + 2 * kBtnYStep, tr(Tr::RadarZoomReset), onReset);
    lv_obj_set_ext_click_area(b, 8);
    layout();
  }

  // Stays true from the start of a pan until tick() sees it end, so ui_manager (100 ms poll) still sees it after the release
  bool blocksSwipe() const override { return swipeBlock_; }

  void tick(uint32_t) override {
    if (!panning_) swipeBlock_ = false;
    if (view_.zoom <= 1.0f) headerTickUtc(hdr_);
    uint32_t v = g_store.aprs.version();
    double la = 0, lo = 0;
    bool me = serviceCoords(la, lo);
    const int32_t rad = cfg().aprs_radius;
    // QTH or radius change moves every station: recompute, not only on new data
    if (v == ver_ && me == me_ && la == la_ && lo == lo_ && rad == rad_) return;
    ver_ = v; me_ = me; la_ = la; lo_ = lo; rad_ = rad;
    n_ = 0;
    g_store.aprs.read([&](const AprsRing& r) {
      for (size_t i = 0; i < r.size() && n_ < MAXN; i++) {
        const AprsStation& s = r.at(i);
        if (!me || !s.hasLatLon) continue;
        Pt& t = pts_[n_++];
        t.distKm = distanceKm(la, lo, s.lat, s.lon);
        t.bearing = bearingDeg(la, lo, s.lat, s.lon);
        t.col = stationColor(s);
        AprsIcon ik; Tr dsc; aprsSymbolInfo(s.sym, ik, dsc); t.icon = ik;
        snprintf(t.call, sizeof t.call, "%s", s.call);
      }
    });
    layout();
    lv_obj_invalidate(cv_);
  }

 private:
  lv_obj_t *hdr_, *cv_;
  RadarView view_ = radarReset();
  Pt pts_[MAXN];
  int n_ = 0;
  uint32_t ver_ = ~0u;
  bool me_ = false;
  double la_ = 0, lo_ = 0;
  int32_t rad_ = -1;
  bool panning_ = false, swipeBlock_ = false;
  lv_point_t last_ = {0, 0};
  Lab labs_[MAXN + 3];
  int nLab_ = 0, fixedLab_ = 0;
  float hdrZoom_ = 1.0f;  // zoom the header text was last set for

  void viewChanged() {
    if (view_.zoom != hdrZoom_) {  // a pan keeps the zoom: leave the header alone
      hdrZoom_ = view_.zoom;
      if (view_.zoom > 1.0f) {
        char b[8];
        snprintf(b, sizeof b, "\xC3\x97%d", (int)view_.zoom);
        headerSetClock(hdr_, b);
      } else {
        headerTickUtc(hdr_);
      }
    }
    layout();
    lv_obj_invalidate(cv_);
  }

  // Adds a label at (x, y) (centred on x when `center`) and returns its index, or -1 when it is optional and would
  // overlap an earlier one, so text never overprints. Boxes of every label added so far (and the buttons) are in `used`.
  int place(const char* str, int x, int y, lv_color_t c, bool center, lv_area_t* used, int& nu, bool optional = false) {
    int w = 0;  // ASCII only
    for (const char* q = str; *q; q++) w += lv_font_get_glyph_width(&font_14, (uint8_t)*q, 0);
    if (center) x -= w / 2;
    lv_area_t a = {x, y, x + w, y + 16};
    for (int i = 0; i < nu; i++) {
      if (hit(used[i], a)) {
        if (optional) return -1;
        break;
      }
    }
    if (nu < MAXN + 8) used[nu++] = a;
    Lab& b = labs_[nLab_];
    b.a = a;
    b.a.x2 += 4;  // slack so the label box never wraps the last glyph
    strlcpy(b.s, str, sizeof b.s);
    b.c = c;
    return nLab_++;
  }

  // Projects the stations and decides which labels fit; onDraw (once per render strip) only replays the result.
  void layout() {
    const float range = (float)cfg().aprs_radius;
    const RadarView& v = view_;
    const float sc = R_PX / range * v.zoom;
    const int qx = (int)lroundf(OX - v.cxKm * sc), qy = (int)lroundf(OY + v.cyKm * sc);  // QTH on screen
    const int rOut = (int)(R_PX * v.zoom);
    const lv_color_t txt = lv_color_hex(0xD3D3D3);
    char b[16];
    lv_area_t used[MAXN + 8];
    int nu = 0;
    nLab_ = 0;
    used[nu++] = {kBtnX - 8, kBtnY0 - 8, SCREEN_W, SCREEN_H};  // the zoom buttons (+, -, 1x)
    formatRadarKm(range, b, sizeof b);
    if (qy - rOut + 2 >= HEADER_H) place(b, qx + 4, qy - rOut + 2, txt, false, used, nu);
    formatRadarKm(range / 2, b, sizeof b);
    if (qy - rOut / 2 + 2 >= HEADER_H) place(b, qx + 4, qy - rOut / 2 + 2, txt, false, used, nu);
    place(tr(Tr::RadarNorth), OX - 16, HEADER_H + 1, txt, false, used, nu, true);  // left of the km labels (at qx + 4)
    fixedLab_ = nLab_;

    const bool labels = v.zoom >= 2 || n_ <= 11;
    for (int i = 0; i < n_; i++) {
      Pt& t = pts_[i];
      t.vis = false;
      t.lab = -1;
      int16_t x, y;
      if (t.distKm <= 0 || t.distKm > range) continue;
      if (!radarProject(v, t.distKm, t.bearing, range, R_PX, OX, OY, x, y)) continue;
      if (y < HEADER_H + 3 || y > SCREEN_H - 4) continue;
      if (x > kBtnX - 16 && y > kBtnY0 - 16) continue;  // keep clear of the zoom buttons
      t.vis = true; t.x = x; t.y = y;
      t.img = aprsIconImage(t.icon, v.zoom <= 1.0f ? 16 : v.zoom < 8.0f ? 24 : 32);
      const int h = t.img ? t.img->header.h : 0;
      t.box = t.img ? lv_area_t{x - h / 2, y - h / 2, x + h / 2 - 1, y + h / 2 - 1} : lv_area_t{x - 4, y - 4, x + 4, y + 4};
      const int up = t.img ? h / 2 + 17 : 18, dn = t.img ? h / 2 + 1 : 4;  // label box is 16 px tall
      if (labels) t.lab = place(t.call, x, y - up < HEADER_H ? y + dn : y - up, t.col, true, used, nu, true);
    }
  }

  static void drawLab(lv_layer_t* l, const Lab& b) {
    if (!hit(l->_clip_area, b.a)) return;
    lv_draw_label_dsc_t d;
    lv_draw_label_dsc_init(&d);
    d.color = b.c; d.font = &font_14; d.text = b.s;  // b.s lives in labs_, stable until the next layout()
    lv_area_t a = b.a;
    lv_draw_label(l, &d, &a);
  }
  lv_obj_t* zoomButton(lv_obj_t* p, int y, const char* txt, lv_event_cb_t cb) {
    lv_obj_t* b = lv_button_create(p);
    lv_obj_set_size(b, kBtnW, kBtnH);
    lv_obj_set_pos(b, kBtnX, y);
    lv_obj_set_style_bg_color(b, lv_color_black(), 0);
    lv_obj_set_style_border_width(b, 1, 0);
    lv_obj_set_style_border_color(b, COL_GREY, 0);
    lv_obj_set_style_radius(b, 6, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, this);
    uiPressFx(b, true);
    lv_obj_t* l = lv_label_create(b);
    lv_obj_set_style_text_font(l, &font_14, 0);
    lv_label_set_text(l, txt);
    lv_obj_center(l);
    return b;
  }
  static void onTap(lv_event_t* e) {
    AprsRadarScreen* s = static_cast<AprsRadarScreen*>(lv_event_get_user_data(e));
    lv_point_t pt;
    lv_indev_get_point(lv_indev_active(), &pt);
    s->view_ = radarZoomAt(s->view_, pt.x, pt.y, (float)cfg().aprs_radius, R_PX, OX, OY);
    s->viewChanged();
  }
  // Long press (400 ms, LVGL default) starts a pan; LVGL then skips SHORT_CLICKED, so no tap zoom follows.
  static void onPan(lv_event_t* e) {
    AprsRadarScreen* s = static_cast<AprsRadarScreen*>(lv_event_get_user_data(e));
    const lv_event_code_t c = lv_event_get_code(e);
    lv_point_t pt;
    if (c == LV_EVENT_PRESSED) {
      s->panning_ = false;  // swipeBlock_ is cleared by tick() once the pan is over
    } else if (c == LV_EVENT_LONG_PRESSED) {
      lv_indev_get_point(lv_indev_active(), &s->last_);
      s->panning_ = s->swipeBlock_ = true;
    } else if (c == LV_EVENT_PRESSING && s->panning_) {
      lv_indev_get_point(lv_indev_active(), &pt);
      if (pt.x == s->last_.x && pt.y == s->last_.y) return;  // finger still: nothing to redraw
      const float k = cfg().aprs_radius / (float)R_PX / s->view_.zoom;  // km per pixel
      s->view_ = radarPan(s->view_, -(pt.x - s->last_.x) * k, (pt.y - s->last_.y) * k, (float)cfg().aprs_radius);
      s->last_ = pt;
      s->viewChanged();
    } else if (c == LV_EVENT_RELEASED || c == LV_EVENT_PRESS_LOST) {
      s->panning_ = false;
    }
  }
  static void step(lv_event_t* e, int dir) {
    AprsRadarScreen* s = static_cast<AprsRadarScreen*>(lv_event_get_user_data(e));
    s->view_ = radarZoomStep(s->view_, dir);
    s->viewChanged();
  }
  static void onZoomIn(lv_event_t* e) { step(e, 1); }
  static void onZoomOut(lv_event_t* e) { step(e, -1); }
  static void onReset(lv_event_t* e) {
    AprsRadarScreen* s = static_cast<AprsRadarScreen*>(lv_event_get_user_data(e));
    s->view_ = radarReset();
    s->viewChanged();
  }

  static void arc(lv_layer_t* l, int cx, int cy, int r, int w, lv_color_t c) {
    lv_draw_arc_dsc_t d;
    lv_draw_arc_dsc_init(&d);
    d.color = c; d.width = w; d.radius = r; d.center = {cx, cy};
    d.start_angle = 0; d.end_angle = 360;
    lv_draw_arc(l, &d);
  }
  // Range ring. lv_draw_arc builds circle masks of ~22 B per pixel of radius in LV_MEM and a failed allocation halts
  // (LV_ASSERT_MALLOC): at 10x the rings are 1000 px, so big rings are drawn as chords (sagitta <= 0.5 px) and only
  // those that touch the layer's current clip area become draw tasks (onDraw runs once per render chunk).
  // Rings that miss the screen are skipped; the angle advances by a rotation, no trig per segment.
  static void ring(lv_layer_t* l, int cx, int cy, int r, lv_color_t c) {
    constexpr int kArcMax = 128, kTop = HEADER_H, kMaxX = SCREEN_W - 1, kMaxY = SCREEN_H - 1;
    int dx = cx < 0 ? -cx : cx > kMaxX ? cx - kMaxX : 0, dy = cy < kTop ? kTop - cy : cy > kMaxY ? cy - kMaxY : 0;
    int fx = cx > SCREEN_W / 2 ? cx : kMaxX - cx, fy = cy > (kTop + kMaxY) / 2 ? cy - kTop : kMaxY - cy;  // farthest corner
    if (r + 1 < (int)hypotf((float)dx, (float)dy) || r - 1 > (int)hypotf((float)fx, (float)fy)) return;
    if (r <= kArcMax) { arc(l, cx, cy, r, 1, c); return; }
    lv_draw_line_dsc_t d;
    lv_draw_line_dsc_init(&d);
    d.color = c; d.width = 1;
    const float chord = 2.0f * sqrtf((float)r);  // sagitta chord^2 / (8 r) = 0.5 px
    const int n = (int)(6.2832f * r / chord) + 1;
    const float da = 6.2832f / n, co = cosf(da), si = sinf(da);
    const lv_area_t& k = l->_clip_area;  // private LVGL field, used on purpose: the public API has no getter for the current chunk
    float vx = (float)r, vy = 0;  // vector from the centre
    for (int i = 0; i < n; i++) {
      float wx = vx * co - vy * si, wy = vx * si + vy * co;
      float x1 = cx + vx, y1 = cy + vy, x2 = cx + wx, y2 = cy + wy;
      if (fmaxf(x1, x2) >= k.x1 - 1 && fminf(x1, x2) <= k.x2 + 1 && fmaxf(y1, y2) >= k.y1 - 1 && fminf(y1, y2) <= k.y2 + 1) {
        d.p1.x = x1; d.p1.y = y1; d.p2.x = x2; d.p2.y = y2;
        lv_draw_line(l, &d);
      }
      vx = wx; vy = wy;
    }
  }
  static void onDraw(lv_event_t* e) {
    AprsRadarScreen* s = static_cast<AprsRadarScreen*>(lv_event_get_user_data(e));
    lv_layer_t* l = lv_event_get_layer(e);
    const RadarView& v = s->view_;
    const float range = (float)cfg().aprs_radius;
    const float sc = R_PX / range * v.zoom;
    const int qx = (int)lroundf(OX - v.cxKm * sc), qy = (int)lroundf(OY + v.cyKm * sc);  // QTH on screen
    const int rOut = (int)(R_PX * v.zoom), rAlert = (int)lroundf(cfg().alrt_dst_km * sc);
    const lv_color_t grey = COL_DIM;

    ring(l, qx, qy, rOut / 2, grey);
    ring(l, qx, qy, rOut, grey);
    if (rAlert >= 1 && rAlert <= 4000) ring(l, qx, qy, rAlert, lv_color_make(70, 130, 190));
    for (int i = 0; i < s->fixedLab_; i++) drawLab(l, s->labs_[i]);
    if (qx >= 0 && qx < SCREEN_W && qy >= HEADER_H && qy < SCREEN_H) arc(l, qx, qy, 4, 4, COL_ACCENT);

    for (int i = 0; i < s->n_; i++) {
      const Pt& t = s->pts_[i];
      if (!t.vis) continue;
      if (hit(l->_clip_area, t.box)) {
        if (t.img) {
          lv_draw_image_dsc_t id;
          lv_draw_image_dsc_init(&id);
          id.src = t.img;
          lv_area_t ia = t.box;
          lv_draw_image(l, &id, &ia);
        } else arc(l, t.x, t.y, 3, 3, t.col);
      }
      if (t.lab >= 0) drawLab(l, s->labs_[t.lab]);
    }
  }
};
}  // namespace

Screen* createAprsRadarScreen() { return new AprsRadarScreen(); }
