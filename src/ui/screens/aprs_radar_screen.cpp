#include "aprs_radar_screen.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "geo.h"
#include "i18n.h"
#include "radar_proj.h"
#include "../../app/config_store.h"
#include "../../app/data_store.h"
#include "../../app/service_coords.h"
#include "../fonts/fonts.h"
#include "../theme.h"
#include "../widgets/header.h"

namespace {
constexpr int R_PX = 100, OX = 160, OY = HEADER_H + (240 - HEADER_H) / 2, MAXN = 30;

struct Pt { float distKm, bearing; lv_color_t col; char call[10]; };

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
    lv_obj_set_size(cv_, 320, 240 - HEADER_H);
    lv_obj_set_pos(cv_, 0, HEADER_H);
    lv_obj_add_flag(cv_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(cv_, onDraw, LV_EVENT_DRAW_MAIN, this);
    lv_obj_add_event_cb(cv_, onTap, LV_EVENT_SHORT_CLICKED, this);

    lv_obj_t* b = lv_button_create(p);
    lv_obj_set_size(b, 44, 28);
    lv_obj_set_pos(b, 272, 208);
    lv_obj_set_ext_click_area(b, 8);
    lv_obj_set_style_bg_color(b, lv_color_black(), 0);
    lv_obj_set_style_border_width(b, 1, 0);
    lv_obj_set_style_border_color(b, lv_color_hex(0xC0C0C0), 0);
    lv_obj_set_style_radius(b, 6, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_add_event_cb(b, onReset, LV_EVENT_CLICKED, this);
    lv_obj_t* l = lv_label_create(b);
    lv_obj_set_style_text_font(l, &font_14, 0);
    lv_label_set_text(l, tr(Tr::RadarZoomReset));
    lv_obj_center(l);
  }

  void tick(uint32_t) override {
    if (view_.zoom <= 1.0f) headerTickUtc(hdr_);
    uint32_t v = g_store.aprs.version();
    double la = 0, lo = 0;
    bool me = serviceCoords(la, lo);
    // QTH or radius change moves every station: recompute, not only on new data
    if (v == ver_ && me == me_ && la == la_ && lo == lo_) return;
    ver_ = v; me_ = me; la_ = la; lo_ = lo;
    n_ = 0;
    g_store.aprs.read([&](const RingBuffer<AprsStation, 30>& r) {
      for (size_t i = 0; i < r.size() && n_ < MAXN; i++) {
        const AprsStation& s = r.at(i);
        if (!me || !s.hasLatLon) continue;
        Pt& t = pts_[n_++];
        t.distKm = distanceKm(la, lo, s.lat, s.lon);
        t.bearing = bearingDeg(la, lo, s.lat, s.lon);
        t.col = stationColor(s);
        snprintf(t.call, sizeof t.call, "%s", s.call);
      }
    });
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

  void viewChanged() {
    if (view_.zoom > 1.0f) {
      char b[8];
      snprintf(b, sizeof b, "\xC3\x97%d", (int)view_.zoom);
      headerSetClock(hdr_, b);
    } else {
      headerTickUtc(hdr_);
    }
    lv_obj_invalidate(cv_);
  }
  static void onTap(lv_event_t* e) {
    AprsRadarScreen* s = static_cast<AprsRadarScreen*>(lv_event_get_user_data(e));
    lv_point_t pt;
    lv_indev_get_point(lv_indev_active(), &pt);
    s->view_ = radarZoomAt(s->view_, pt.x, pt.y, (float)cfg().aprs_radius, R_PX, OX, OY);
    s->viewChanged();
  }
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
  // (LV_ASSERT_MALLOC): at 8x the rings are 800 px, so big rings are drawn as chords (sagitta <= 0.5 px) and only
  // those that touch the layer's current clip area become draw tasks (onDraw runs once per render chunk).
  // Rings that miss the screen are skipped; the angle advances by a rotation, no trig per segment.
  static void ring(lv_layer_t* l, int cx, int cy, int r, lv_color_t c) {
    constexpr int kArcMax = 128, kTop = HEADER_H;
    int dx = cx < 0 ? -cx : cx > 319 ? cx - 319 : 0, dy = cy < kTop ? kTop - cy : cy > 239 ? cy - 239 : 0;
    int fx = cx > 160 ? cx : 319 - cx, fy = cy > (kTop + 239) / 2 ? cy - kTop : 239 - cy;  // farthest corner
    if (r + 1 < (int)hypotf((float)dx, (float)dy) || r - 1 > (int)hypotf((float)fx, (float)fy)) return;
    if (r <= kArcMax) { arc(l, cx, cy, r, 1, c); return; }
    lv_draw_line_dsc_t d;
    lv_draw_line_dsc_init(&d);
    d.color = c; d.width = 1;
    const float chord = 2.0f * sqrtf((float)r);  // sagitta chord^2 / (8 r) = 0.5 px
    const int n = (int)(6.2832f * r / chord) + 1;
    const float da = 6.2832f / n, co = cosf(da), si = sinf(da);
    const lv_area_t& k = l->_clip_area;
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
  // Draws s at (x, y) (centred on x when `center`) and records its box in `used`. Optional labels
  // (`optional`) are skipped when they would overlap an earlier one, so text never overprints.
  static void text(lv_layer_t* l, const char* s, int x, int y, lv_color_t c, bool center, lv_area_t* used, int& nu,
                   bool optional = false) {
    int w = 0;  // ASCII only
    for (const char* q = s; *q; q++) w += lv_font_get_glyph_width(&font_14, (uint8_t)*q, 0);
    if (center) x -= w / 2;
    lv_area_t a = {x, y, x + w, y + 16};
    for (int i = 0; i < nu; i++) {
      const lv_area_t& u = used[i];
      if (a.x1 <= u.x2 && u.x1 <= a.x2 && a.y1 <= u.y2 && u.y1 <= a.y2) {
        if (optional) return;
        break;
      }
    }
    if (nu < MAXN + 8) used[nu++] = a;
    lv_draw_label_dsc_t d;
    lv_draw_label_dsc_init(&d);
    d.color = c; d.font = &font_14; d.text = s; d.text_local = 1;  // copied: s may be a stack buffer
    a.x2 += 4;  // slack so the label box never wraps the last glyph
    lv_draw_label(l, &d, &a);
  }
  static void km(char* b, size_t n, float v) {
    if (v >= 100) snprintf(b, n, "%dkm", (int)(v + 0.5f));
    else if (v >= 10) snprintf(b, n, "%.1fkm", (double)v);
    else snprintf(b, n, "%.2fkm", (double)v);
  }

  static void onDraw(lv_event_t* e) {
    AprsRadarScreen* s = static_cast<AprsRadarScreen*>(lv_event_get_user_data(e));
    lv_layer_t* l = lv_event_get_layer(e);
    const RadarView& v = s->view_;
    const float range = (float)cfg().aprs_radius;
    const float sc = R_PX / range * v.zoom;
    const int qx = (int)lroundf(OX - v.cxKm * sc), qy = (int)lroundf(OY + v.cyKm * sc);  // QTH on screen
    const int rOut = (int)(R_PX * v.zoom), rAlert = (int)lroundf(cfg().alrt_dst_km * sc);
    const lv_color_t grey = lv_color_hex(0x808080), txt = lv_color_hex(0xD3D3D3);

    ring(l, qx, qy, rOut / 2, grey);
    ring(l, qx, qy, rOut, grey);
    if (rAlert >= 1 && rAlert <= 2000) ring(l, qx, qy, rAlert, lv_color_make(70, 130, 190));
    char b[16];
    lv_area_t used[MAXN + 8];
    int nu = 0;
    used[nu++] = {272 - 8, 208 - 8, 320, 240};  // the 1x button (with its extended click area)
    km(b, sizeof b, range);
    if (qy - rOut + 2 >= HEADER_H) text(l, b, qx + 4, qy - rOut + 2, txt, false, used, nu);
    km(b, sizeof b, range / 2);
    if (qy - rOut / 2 + 2 >= HEADER_H) text(l, b, qx + 4, qy - rOut / 2 + 2, txt, false, used, nu);
    text(l, tr(Tr::RadarNorth), OX - 16, HEADER_H + 1, txt, false, used, nu, true);  // left of the km labels (at qx + 4)
    if (qx >= 0 && qx < 320 && qy >= HEADER_H && qy < 240) arc(l, qx, qy, 4, 4, lv_color_hex(0xFF8C00));

    const bool labels = v.zoom >= 2 || s->n_ <= 11;
    for (int i = 0; i < s->n_; i++) {
      const Pt& t = s->pts_[i];
      int16_t x, y;
      if (t.distKm <= 0 || t.distKm > range) continue;
      if (!radarProject(v, t.distKm, t.bearing, range, R_PX, OX, OY, x, y)) continue;
      if (y < HEADER_H + 3 || y > 236) continue;
      if (x > 256 && y > 192) continue;  // keep clear of the 1\xC3\x97 button
      arc(l, x, y, 3, 3, t.col);
      if (labels) text(l, t.call, x, y - 18 < HEADER_H ? y + 4 : y - 18, t.col, true, used, nu, true);
    }
  }
};
}  // namespace

Screen* createAprsRadarScreen() { return new AprsRadarScreen(); }
