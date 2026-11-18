#include "alert_overlay.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include "aprs_symbol.h"
#include "fifo_queue.h"
#include "aprs_wx.h"
#include "i18n.h"

#include "../../app/config_store.h"
#include "../fonts/fonts.h"
#include "../icons/weather_icons.h"
#include "../theme.h"
#include "../widgets/aprs_icon.h"
#include "../widgets/ui_kit.h"

static lv_obj_t* ov;
static uint32_t deadline;
static bool closeReq;
static lv_obj_t* frame;
static int lastPulse = -1;
static FifoQueue<UiEvent, 4> pending;  // alerts that arrived while an overlay was showing (UI task only)
static void show(const UiEvent&);

static void onClose(lv_event_t*) { closeReq = true; }  // delete happens in overlayTick

static void closeNow() {
  if (ov) lv_obj_delete(ov);
  ov = nullptr;
  frame = nullptr;
  lastPulse = -1;
  closeReq = false;
}


namespace {
// Appends to a NUL-terminated buffer; never writes past cap (C-09).
__attribute__((format(printf, 3, 4))) void appendf(char* buf, size_t cap, const char* fmt, ...) {
  size_t n = strnlen(buf, cap);
  if (n + 1 >= cap) return;
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf + n, cap - n, fmt, ap);
  va_end(ap);
}

void box(lv_layer_t* ly, int x1, int y1, int x2, int y2, lv_color_t c, int r) {
  lv_draw_rect_dsc_t d;
  lv_draw_rect_dsc_init(&d);
  d.bg_color = c;
  d.radius = r;
  lv_area_t a = {x1, y1, x2, y2};
  lv_draw_rect(ly, &d, &a);
}

void line(lv_layer_t* ly, int x1, int y1, int x2, int y2, lv_color_t c, int w) {
  lv_draw_line_dsc_t d;
  lv_draw_line_dsc_init(&d);
  d.color = c;
  d.width = w;
  d.round_start = d.round_end = 1;
  d.p1.x = x1; d.p1.y = y1; d.p2.x = x2; d.p2.y = y2;
  lv_draw_line(ly, &d);
}

void aprsImage(AprsIcon ik, int x, int y) {
  lv_obj_t* im = lv_image_create(ov);
  lv_image_set_src(im, aprsIconImage(ik, 48));
  lv_obj_set_pos(im, x, y);
}

void onCompassDraw(lv_event_t* e) {
  lv_obj_t* o = lv_event_get_target_obj(e);
  lv_layer_t* ly = lv_event_get_layer(e);
  lv_area_t a;
  lv_obj_get_coords(o, &a);
  int cx = a.x1 + 30, cy = a.y1 + 33;
  lv_draw_arc_dsc_t ad;
  lv_draw_arc_dsc_init(&ad);
  ad.color = lv_color_hex(0x808080);
  ad.width = 1;
  ad.center.x = cx; ad.center.y = cy;
  ad.start_angle = 0; ad.end_angle = 360;
  ad.radius = 24;
  lv_draw_arc(ly, &ad);
  ad.radius = 23;
  lv_draw_arc(ly, &ad);
  for (int i = 0; i < 4; i++) {  // N/E/S/W ticks
    int dx = (i == 1) - (i == 3), dy = (i == 2) - (i == 0);
    line(ly, cx + dx * 20, cy + dy * 20, cx + dx * 24, cy + dy * 24, lv_color_hex(0xC0C0C0), 1);
  }
  int b = (int)(intptr_t)lv_obj_get_user_data(o);
  if (b < 0) return;
  float r = b * (float)M_PI / 180.f;
  int tx = cx + (int)lroundf(20 * sinf(r)), ty = cy - (int)lroundf(20 * cosf(r));
  line(ly, cx, cy, tx, ty, lv_color_hex(0x00FFFF), 2);
  box(ly, tx - 2, ty - 2, tx + 2, ty + 2, lv_color_hex(0x00FFFF), LV_RADIUS_CIRCLE);
}

lv_obj_t* drawObj(lv_obj_t* p, int x, int y, int w, int h, lv_event_cb_t cb, intptr_t ud) {
  lv_obj_t* o = lv_obj_create(p);
  lv_obj_remove_style_all(o);
  lv_obj_set_pos(o, x, y);
  lv_obj_set_size(o, w, h);
  lv_obj_set_user_data(o, (void*)ud);
  lv_obj_add_event_cb(o, cb, LV_EVENT_DRAW_MAIN, nullptr);
  return o;
}

void buildAprs(lv_obj_t* ov, const AprsStation& st) {
  const lv_color_t white = COL_TEXT, grey = lv_color_hex(0xC0C0C0), cyan = lv_color_hex(0x00FFFF);
  frame = lv_obj_create(ov);
  lv_obj_remove_style_all(frame);
  lv_obj_set_pos(frame, 6, 44);
  lv_obj_set_size(frame, 308, 190);
  lv_obj_set_style_radius(frame, 8, 0);
  lv_obj_set_style_border_width(frame, 3, 0);
  lv_obj_set_style_border_color(frame, white, 0);
  lv_obj_remove_flag(frame, LV_OBJ_FLAG_CLICKABLE);

  char b[48];
  uiLabel(ov, &font_24, white, st.call[0] ? st.call : "-", 18, 52);
  if (st.freqMhz > 0) snprintf(b, sizeof b, "%.3f %s", (double)st.freqMhz, tr(Tr::Mhz));
  else snprintf(b, sizeof b, "-");
  uiLabel(ov, &font_14, grey, b, 18, 82);

  // compass 60x60 + bearing/distance under it
  int bearing = st.hasLatLon && st.bearing >= 0 ? st.bearing : -1;
  drawObj(ov, 236, 50, 60, 66, onCompassDraw, bearing);
  if (bearing >= 0) snprintf(b, sizeof b, "%03d\xC2\xB0", bearing);
  else snprintf(b, sizeof b, "---");
  uiLabel(ov, &font_14, cyan, b, 236, 118, 60, LV_TEXT_ALIGN_CENTER);
  if (st.hasLatLon && st.distKm >= 0) snprintf(b, sizeof b, "%d %s", (int)st.distKm, tr(Tr::Km));
  else snprintf(b, sizeof b, "- %s", tr(Tr::Km));
  uiLabel(ov, &font_14, COL_ACCENT, b, 226, 134, 80, LV_TEXT_ALIGN_CENTER);

  AprsWx wx{};
  if (st.isWx && aprsDecodeWx(st.comment, wx)) {
    int y = 106;
    if (wx.hasTempC) { snprintf(b, sizeof b, "%s %.1f C", tr(Tr::WxTemp), (double)wx.tempC); uiLabel(ov, &font_14, lv_color_hex(0x4080FF), b, 18, y); y += 18; }
    if (wx.hasHumidity) { snprintf(b, sizeof b, "%s %d %%", tr(Tr::HumidityLabel), wx.humidityPct); uiLabel(ov, &font_14, white, b, 18, y); y += 18; }
    if (wx.hasPressure) { snprintf(b, sizeof b, "%s %.1f %s", tr(Tr::PressureLabel), (double)wx.pressureHpa, tr(Tr::HPa)); uiLabel(ov, &font_14, white, b, 18, y); }
    if (wx.hasWindSpeed) {
      if (wx.hasWindDir) snprintf(b, sizeof b, "%s %d\xC2\xB0 %.1f %s", tr(Tr::WxWind), wx.windDirDeg, (double)wx.windSpeedKmh, tr(Tr::KmH));
      else snprintf(b, sizeof b, "%s %.1f %s", tr(Tr::WxWind), (double)wx.windSpeedKmh, tr(Tr::KmH));
      uiLabel(ov, &font_14, lv_color_hex(0x00FF00), b, 18, 160);
    }
  } else {
    AprsIcon ik;
    Tr d;
    aprsSymbolInfo(st.sym, ik, d);
    if (ik != AprsIcon::None) aprsImage(ik, 18, 104);
    else uiLabel(ov, &font_18, white, tr(d), 18, 116);
  }
  if (st.comment[0] && !(st.isWx && wx.hasAny)) {
    lv_obj_t* c = uiLabel(ov, &font_14, grey, st.comment, 18, 176);
    lv_obj_set_width(c, 284);
    lv_label_set_long_mode(c, LV_LABEL_LONG_WRAP);
    lv_obj_set_height(c, 38);
  }
}
}  // namespace

bool overlayActive() { return ov != nullptr; }

void overlayShow(const UiEvent& e) {
  if (ov) pending.push(e);  // never replace the visible overlay; queue full drops the oldest queued one
  else show(e);
}

static void show(const UiEvent& e) {
  static char body[160];  // static: keeps 160 B off the UI task stack (UI task only)
  const char* title;
  const lv_image_dsc_t* wxImg = nullptr;
  bool aprs = false;
  AprsIcon bIcon = AprsIcon::None;
  uint32_t secs = (uint32_t)cfg().alrt_scr_s;
  body[0] = 0;
  switch (e.kind) {
    case EventKind::AprsAlert:
      title = tr(Tr::AprsAlertTitle);
      aprs = true;
      break;
    case EventKind::Beaconing: {
      title = tr(Tr::AprsBeaconing);
      Tr d;
      aprsSymbolInfo(e.beacon.sym, bIcon, d);
      if (bIcon == AprsIcon::None) appendf(body, sizeof body, "%s\n", tr(d));
      appendf(body, sizeof body, "%s\n%s\n%s", e.beacon.call, e.beacon.position, e.beacon.path);
      secs = 30;  // fixed, not configurable
      break;
    }
    case EventKind::WeatherAlert:
      title = tr(Tr::WeatherAlert);
      if (e.wx.stormPossible) {
        wxImg = &wxicon_200;
        appendf(body, sizeof body, "%s\n", tr(e.wx.stormNow ? Tr::StormNow : Tr::PossibleStorm));
      }
      if (e.wx.windPossible) {
        if (!wxImg) wxImg = &wxicon_wind;
        appendf(body, sizeof body, "%s\n%s %.1f %s\n", tr(Tr::StrongWind), tr(Tr::WindLabel), (double)e.wx.maxWindMs, tr(Tr::Ms));
        if (e.wx.maxWindGustMs > 0)
          appendf(body, sizeof body, "%s %.1f %s\n", tr(Tr::Gust), (double)e.wx.maxWindGustMs, tr(Tr::Ms));
      }
      // stormEtaHours carries the storm repeat interval in minutes; wind alone repeats every minute
      appendf(body, sizeof body, "%s %u %s", tr(Tr::Repeat), e.wx.stormPossible ? (unsigned)e.wx.stormEtaHours : 1u, tr(Tr::Min));
      break;
    default:
      return;
  }
  if (secs < 1) secs = 1;
  if (secs > 60) secs = 60;
  deadline = millis() + secs * 1000;

  ov = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(ov);
  lv_obj_set_size(ov, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(ov, COL_BG, 0);
  lv_obj_set_style_bg_opa(ov, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(ov, COL_TEXT, 0);
  lv_obj_add_flag(ov, LV_OBJ_FLAG_CLICKABLE);  // swallows touches
  lv_obj_remove_flag(ov, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* t = uiLabel(ov, &font_24, COL_ACCENT, title);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, aprs ? 8 : 42);  // non-APRS titles can be wide: stay below the X button (y<=40)

  if (aprs) {
    buildAprs(ov, e.station);
  } else {
    lv_obj_t* b = uiLabel(ov, &font_18, COL_TEXT, body, 0, 0, 290, LV_TEXT_ALIGN_CENTER);
    if (wxImg || bIcon != AprsIcon::None) {  // icon on the left, text left-aligned beside it
      if (wxImg) {
        lv_obj_t* im = lv_image_create(ov);
        lv_image_set_src(im, wxImg);
        lv_obj_set_pos(im, 18, 90);
      } else {
        aprsImage(bIcon, 18, 90);
      }
      lv_obj_set_width(b, 226);
      lv_obj_set_style_text_align(b, LV_TEXT_ALIGN_LEFT, 0);
      lv_obj_align(b, LV_ALIGN_TOP_LEFT, 82, 76);  // top-anchored: up to 6 lines never reach the title
    } else {
      lv_obj_align(b, LV_ALIGN_CENTER, 0, 10);
    }
  }

  lv_obj_t* x = lv_button_create(ov);
  lv_obj_set_size(x, 44, 36);
  lv_obj_align(x, LV_ALIGN_TOP_RIGHT, -4, 4);
  lv_obj_add_event_cb(x, onClose, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* xl = lv_label_create(x);
  lv_label_set_text(xl, "X");
  lv_obj_center(xl);
}

void overlayTick(uint32_t nowMs) {
  if (ov && frame) {  // 500 ms pulse
    int p = (nowMs / 500) & 1;
    if (p != lastPulse) {
      lastPulse = p;
      lv_obj_set_style_border_color(frame, p ? lv_color_hex(0xC0C0C0) : COL_TEXT, 0);
      lv_obj_set_style_border_width(frame, p ? 1 : 3, 0);
    }
  }
  if (ov && (closeReq || (int32_t)(nowMs - deadline) >= 0)) {
    closeNow();
    UiEvent next;
    if (pending.pop(next)) show(next);
  }
}
