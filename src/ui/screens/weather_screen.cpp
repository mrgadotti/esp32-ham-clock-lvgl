#include "weather_screen.h"

#include <stdio.h>
#include <string.h>

#include "i18n.h"
#include "weather_json.h"
#include "wx_common.h"
#include "../../app/config_store.h"
#include "../../app/data_store.h"
#include "../fonts/fonts.h"
#include "../theme.h"
#include "../widgets/header.h"

namespace {
#define CYAN lv_color_hex(0x00FFFF)
#define GREY lv_color_hex(0x909090)

lv_color_t pmColor(float v, float a, float b, float c, float d) {
  if (v <= a) return COL_MODE;
  if (v <= b) return COL_FREQ;
  if (v <= c) return lv_color_hex(0xFF7F00);
  if (v <= d) return COL_ERROR;
  return lv_color_hex(0xF80000);
}

class WeatherScreen : public Screen {
 public:
  ScreenId id() const override { return ScreenId::Weather; }

  void create(lv_obj_t* p) override {
    hdr_ = headerCreate(p, Tr::Weather);
    tLbl_ = wxLabel(p, &font_14, GREY, 15, 32);
    lv_label_set_text(tLbl_, tr(Tr::Temperature));
    temp_ = wxLabel(p, &font_24, CYAN, 15, 50);
    icon_ = wxIconCreate(p, 215, 32);
    desc_ = wxLabel(p, &font_14, COL_TEXT, 160, 86, 160, LV_TEXT_ALIGN_CENTER);
    const Tr names[3] = {Tr::Humidity, Tr::Pressure, Tr::Wind};
    for (int i = 0; i < 3; i++) {
      lbl_[i] = wxLabel(p, &font_14, GREY, 15, 128 + i * 26);
      lv_label_set_text(lbl_[i], tr(names[i]));
      val_[i] = wxLabel(p, &font_18, COL_TEXT, 110, 124 + i * 26);
    }
    pm25_ = wxLabel(p, &font_18, COL_TEXT, 15, 198);
    pm10_ = wxLabel(p, &font_18, COL_TEXT, 165, 198);
    foot_ = wxLabel(p, &font_14, lv_color_hex(0x52AA52), 0, 222, 320, LV_TEXT_ALIGN_CENTER);
    refresh(true);
  }

  void tick(uint32_t) override { refresh(false); }

 private:
  lv_obj_t *hdr_, *tLbl_, *temp_, *icon_, *desc_, *lbl_[3], *val_[3], *pm25_, *pm10_, *foot_;
  uint32_t ver_ = ~0u;
  char clock_[8] = "", foot2_[96] = "";

  void refresh(bool force) {
    char clk[8];
    wxLocalClock(clk, sizeof clk);
    if (strcmp(clk, clock_) != 0) {
      strcpy(clock_, clk);
      headerSetClock(hdr_, clk);
      force = true;  // footer age follows the minute
    }
    uint32_t v = g_store.weather.version();
    if (!force && v == ver_) return;
    ver_ = v;
    WeatherData w;
    g_store.weather.read([&](const WeatherData& s) { w = s; });
    char b[96];
    wxFooter(b, sizeof b, w, cfg().weather_key[0] != 0);
    if (strcmp(b, foot2_) != 0) { strcpy(foot2_, b); lv_label_set_text(foot_, b); }
    lv_obj_t* body[] = {tLbl_, temp_, icon_, desc_, lbl_[0], lbl_[1], lbl_[2], val_[0], val_[1], val_[2], pm25_, pm10_};
    for (lv_obj_t* o : body) lv_obj_set_flag(o, LV_OBJ_FLAG_HIDDEN, !w.valid);
    if (!w.valid) return;
    lv_label_set_text_fmt(temp_, "%.1f \xC2\xB0""C", (double)w.tempC);
    wxIconSet(icon_, w.weatherId, isNightFromIcon(w.iconCode));
    lv_label_set_text(desc_, w.description);
    lv_label_set_text_fmt(val_[0], "%d%%", w.humidity);
    lv_label_set_text_fmt(val_[1], "%d %s", w.pressure, tr(Tr::HPa));
    lv_label_set_text_fmt(val_[2], "%.1f %s", (double)w.windMs, tr(Tr::Ms));
    lv_obj_set_style_text_color(val_[2], wxWindColor(w.windMs), 0);
    lv_label_set_text_fmt(pm25_, "PM2.5: %.1f", (double)w.pm25);
    lv_obj_set_style_text_color(pm25_, pmColor(w.pm25, 12, 35, 55, 150), 0);
    lv_label_set_text_fmt(pm10_, "PM10: %.1f", (double)w.pm10);
    lv_obj_set_style_text_color(pm10_, pmColor(w.pm10, 20, 50, 100, 200), 0);
  }
};
}  // namespace

Screen* createWeatherScreen() { return new WeatherScreen(); }
