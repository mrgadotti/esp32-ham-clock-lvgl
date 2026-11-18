#include "forecast_screen.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "i18n.h"
#include "weather_json.h"
#include "wx_common.h"
#include "../../app/config_store.h"
#include "../../app/data_store.h"
#include "../fonts/fonts.h"
#include "../theme.h"
#include "../widgets/header.h"
#include "../widgets/ui_kit.h"

namespace {
constexpr int COLS = 4, COL_W = 79;
#define CYAN COL_CYAN
#define GREY lv_color_hex(0x909090)

class ForecastScreen : public Screen {
 public:
  explicit ForecastScreen(bool om) : om_(om) {}
  ScreenId id() const override { return om_ ? ScreenId::ForecastOm : ScreenId::Forecast; }

  void create(lv_obj_t* p) override {
    hdr_ = headerCreate(p, om_ ? Tr::ForecastOm : Tr::Forecast);
    for (int c = 0; c < COLS; c++) {
      int x = 2 + c * COL_W;
      if (c) {  // column separator
        lv_obj_t* l = lv_obj_create(p);
        lv_obj_remove_style_all(l);
        lv_obj_set_size(l, 1, 180);
        lv_obj_set_pos(l, x - 1, 32);
        lv_obj_set_style_bg_color(l, lv_color_hex(0x404040), 0);
        lv_obj_set_style_bg_opa(l, LV_OPA_COVER, 0);
      }
      head_[c] = uiLabel(p, &font_18, COL_ACCENT, "", x, 32, COL_W, LV_TEXT_ALIGN_CENTER);
      temp_[c] = uiLabel(p, &font_18, CYAN, "", x, 60, COL_W, LV_TEXT_ALIGN_CENTER);
      night_[c] = uiLabel(p, &font_14, GREY, "", x, 84, COL_W, LV_TEXT_ALIGN_CENTER);
      hum_[c] = uiLabel(p, &font_18, COL_TEXT, "", x, 104, COL_W, LV_TEXT_ALIGN_CENTER);
      wind_[c] = uiLabel(p, &font_14, COL_TEXT, "", x, 132, COL_W, LV_TEXT_ALIGN_CENTER);
      icon_[c] = wxIconCreate(p, x + (COL_W - 50) / 2, 158);
    }
    msg_ = uiLabel(p, &font_18, COL_ERROR, "", 12, 90, 296);
    foot_ = uiLabel(p, &font_14, lv_color_hex(0x52AA52), "", 0, 222, SCREEN_W, LV_TEXT_ALIGN_CENTER);
    lv_label_set_long_mode(foot_, LV_LABEL_LONG_DOT);
    lv_obj_set_height(foot_, 18);
    refresh(true);
  }

  void tick(uint32_t) override { refresh(false); }

 private:
  lv_obj_t *hdr_, *head_[COLS], *temp_[COLS], *night_[COLS], *hum_[COLS], *wind_[COLS], *icon_[COLS], *msg_, *foot_;
  const bool om_;
  uint32_t ver_ = ~0u;
  char clock_[8] = "", foot2_[96] = "";
  WeatherData w_;  // member, not a stack local (K-03)

  void refresh(bool force) {
    char clk[8];
    struct tm tm;
    wxLocalClock(clk, sizeof clk, &tm);
    if (strcmp(clk, clock_) != 0) {
      strcpy(clock_, clk);
      headerSetClock(hdr_, clk);
      force = true;
    }
    uint32_t v = (om_ ? g_store.weatherOm : g_store.weather).version();
    if (!force && v == ver_) return;
    ver_ = v;
    WeatherData& w = w_;
    (om_ ? g_store.weatherOm : g_store.weather).read([&](const WeatherData& s) { w = s; });
    const bool haveKey = om_ || cfg().weather_key[0] != 0;  // Open-Meteo needs no key
    char b[96];
    if (om_) wxFooterOm(b, sizeof b, w, true);
    else wxFooter(b, sizeof b, w, haveKey, true);
    if (strcmp(b, foot2_) != 0) { strcpy(foot2_, b); lv_label_set_text(foot_, b); }

    bool any = false;
    for (int c = 0; c < COLS; c++) any |= w.detailValid[c];
    if (!any) {
      lv_label_set_text(msg_, tr(!haveKey ? Tr::NoApiKey
                                 : w.forecastFetchFailed ? Tr::ForecastNoData
                                 : w.fetchFailed || !w.valid ? Tr::NoData : Tr::NoForecast));
      lv_obj_set_style_text_color(msg_, haveKey && w.valid && !w.forecastFetchFailed ? COL_FREQ : COL_ERROR, 0);
    }
    lv_obj_set_flag(msg_, LV_OBJ_FLAG_HIDDEN, any);
    for (int c = 0; c < COLS; c++) {
      bool ok = any && w.detailValid[c];
      lv_obj_t* cells[] = {head_[c], temp_[c], night_[c], hum_[c], wind_[c], icon_[c]};
      for (lv_obj_t* o : cells) lv_obj_set_flag(o, LV_OBJ_FLAG_HIDDEN, !any);
      if (!any) continue;
      if (c == 0) lv_label_set_text(head_[c], tr(Tr::Plus3h));
      else if (c == 1) lv_label_set_text(head_[c], tr(Tr::Plus6h));
      else if (tm.tm_year) lv_label_set_text(head_[c], tr((Tr)((int)Tr::DaySun + (tm.tm_wday + c - 1) % 7)));
      else lv_label_set_text_fmt(head_[c], "D+%d", c - 1);
      if (!ok) {
        lv_label_set_text(temp_[c], "--");
        lv_label_set_text(night_[c], "");
        lv_label_set_text(hum_[c], "--");
        lv_label_set_text(wind_[c], "--");
        lv_obj_add_flag(icon_[c], LV_OBJ_FLAG_HIDDEN);
        continue;
      }
      lv_label_set_text_fmt(temp_[c], "%.0f\xC2\xB0", (double)w.detailTempC[c]);
      if (c >= 2 && w.nightTempValid[c - 2])
        lv_label_set_text_fmt(night_[c], "%s %.0f", tr(Tr::ColNight), (double)w.nightTempC[c - 2]);
      else lv_label_set_text(night_[c], c >= 2 ? "--" : "");
      lv_label_set_text_fmt(hum_[c], "%d%%", w.detailHumidity[c]);
      lv_label_set_text_fmt(wind_[c], "%.1f%s", (double)w.detailWindMs[c], tr(Tr::Ms));
      lv_obj_set_style_text_color(wind_[c], wxWindColor(w.detailWindMs[c]), 0);
      wxIconSet(icon_[c], w.detailWeatherId[c], w.detailNight[c]);
    }
  }
};
}  // namespace

Screen* createForecastScreen(bool openMeteo) { return new ForecastScreen(openMeteo); }
