#include "alert_overlay.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include "i18n.h"

#include "../../app/config_store.h"
#include "../fonts/fonts.h"
#include "../icons/weather_icons.h"
#include "../theme.h"

static lv_obj_t* ov;
static uint32_t deadline;
static bool closeReq;

static void onClose(lv_event_t*) { closeReq = true; }  // delete happens in overlayTick

static void closeNow() {
  if (ov) lv_obj_delete(ov);
  ov = nullptr;
  closeReq = false;
}

bool overlayActive() { return ov != nullptr; }

void overlayShow(const UiEvent& e) {
  closeNow();
  char title[40], body[160];
  const lv_image_dsc_t* wxImg = nullptr;
  uint32_t secs = (uint32_t)cfg().alrt_scr_s;
  switch (e.kind) {
    case EventKind::AprsAlert:
      snprintf(title, sizeof title, "%s", tr(Tr::AprsAlertTitle));
      snprintf(body, sizeof body, "%s\n%s", e.station.call, e.station.comment);
      break;
    case EventKind::Beaconing:
      snprintf(title, sizeof title, "%s", tr(Tr::AprsBeaconing));
      snprintf(body, sizeof body, "%s\n%s", e.beacon.call, e.beacon.position);
      secs = 2;
      break;
    case EventKind::WeatherAlert:
      snprintf(title, sizeof title, "%s", tr(Tr::WeatherAlert));
      body[0] = 0;
      if (e.wx.stormPossible) {
        wxImg = &wxicon_200;
        snprintf(body, sizeof body, "%s\n", tr(e.wx.stormNow ? Tr::StormNow : Tr::PossibleStorm));
      }
      if (e.wx.windPossible) {
        if (!wxImg) wxImg = &wxicon_wind;
        size_t n = strlen(body);
        n += snprintf(body + n, sizeof body - n, "%s\n%s %.1f %s\n", tr(Tr::StrongWind), tr(Tr::WindLabel), (double)e.wx.maxWindMs, tr(Tr::Ms));
        if (e.wx.maxWindGustMs > 0)
          snprintf(body + n, sizeof body - n, "%s %.1f %s\n", tr(Tr::Gust), (double)e.wx.maxWindGustMs, tr(Tr::Ms));
      }
      {  // stormEtaHours carries the storm repeat interval in minutes; wind alone repeats every minute
        size_t n = strlen(body);
        snprintf(body + n, sizeof body - n, "%s %u %s", tr(Tr::Repeat), e.wx.stormPossible ? (unsigned)e.wx.stormEtaHours : 1u, tr(Tr::Min));
      }
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

  lv_obj_t* t = lv_label_create(ov);
  lv_obj_set_style_text_font(t, &font_24, 0);
  lv_obj_set_style_text_color(t, COL_ACCENT, 0);
  lv_label_set_text(t, title);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 30);

  lv_obj_t* b = lv_label_create(ov);
  lv_obj_set_style_text_font(b, &font_18, 0);
  lv_obj_set_style_text_align(b, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_width(b, 290);
  lv_label_set_text(b, body);
  if (wxImg) {  // weather alert: icon on the left, text left-aligned beside it
    lv_obj_t* im = lv_image_create(ov);
    lv_image_set_src(im, wxImg);
    lv_obj_set_pos(im, 18, 90);
    lv_obj_set_width(b, 220);
    lv_obj_set_style_text_align(b, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(b, LV_ALIGN_LEFT_MID, 82, 15);
  } else {
    lv_obj_align(b, LV_ALIGN_CENTER, 0, 10);
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
  if (ov && (closeReq || (int32_t)(nowMs - deadline) >= 0)) closeNow();
}
