#include "../screen.h"
#include "aprs_list_screen.h"
#include "clock_screen.h"
#include "dx_screen.h"
#include "hamalert_screen.h"
#include "pota_screen.h"
#include "settings_screen.h"
#include "weather_screen.h"
#include "forecast_screen.h"
#include "propagation_screen.h"
#include "bands_screen.h"
#include "matrix_screen.h"
#include "aprs_radar_screen.h"
#include "../fonts/fonts.h"
#include "../widgets/header.h"
#include "i18n.h"

namespace {
class PlaceholderScreen : public Screen {
 public:
  explicit PlaceholderScreen(ScreenId id) : id_(id) {}
  ScreenId id() const override { return id_; }
  void create(lv_obj_t* p) override {
    headerCreate(p, title(id_));
    lv_obj_t* l = lv_label_create(p);
    lv_obj_set_style_text_font(l, &font_18, 0);
    lv_label_set_text(l, "coming soon");
    lv_obj_center(l);
  }
  void tick(uint32_t) override {}

 private:
  static Tr title(ScreenId id) {
    switch (id) {
      case ScreenId::DxCluster: return Tr::DxCluster;
      case ScreenId::SunSpots: return Tr::Propagation;
      case ScreenId::BandInfo: return Tr::HfBands;
      case ScreenId::Weather: return Tr::Weather;
      case ScreenId::AprsList: return Tr::AprsIs;
      case ScreenId::Pota: return Tr::Pota;
      case ScreenId::HamAlert: return Tr::HamAlert;
      case ScreenId::AprsRadar: return Tr::AprsRadar;
      case ScreenId::Matrix: return Tr::Matrix;
      case ScreenId::Game: return Tr::GameTitle;
      case ScreenId::Forecast: return Tr::Forecast;
      case ScreenId::Location: return Tr::Location;
      case ScreenId::Settings: return Tr::Settings;
      default: return Tr::Clock;
    }
  }
  ScreenId id_;
};
}  // namespace

Screen* createScreen(ScreenId id) {
  if (id == ScreenId::HamClock) return createClockScreen();
  if (id == ScreenId::DxCluster) return createDxScreen();
  if (id == ScreenId::AprsList) return createAprsListScreen();
  if (id == ScreenId::Pota) return createPotaScreen();
  if (id == ScreenId::HamAlert) return createHamAlertScreen();
  if (id == ScreenId::SunSpots) return createPropagationScreen();
  if (id == ScreenId::BandInfo) return createBandsScreen();
  if (id == ScreenId::Settings) return createSettingsScreen();
  if (id == ScreenId::Weather) return createWeatherScreen();
  if (id == ScreenId::Forecast) return createForecastScreen();
  if (id == ScreenId::Matrix) return createMatrixScreen();
  if (id == ScreenId::AprsRadar) return createAprsRadarScreen();
  return new PlaceholderScreen(id);
}
