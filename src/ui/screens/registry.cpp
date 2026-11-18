#include "carousel.h"
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
#include "aprs_radar_screen.h"

// carousel.h keeps a copy of the ids (lib/core has no LVGL); fail the build if they drift.
#define SCR_CHECK(n) static_assert(static_cast<uint8_t>(ScreenId::n) == scr::n, "scr::" #n " out of sync with ScreenId")
SCR_CHECK(HamClock); SCR_CHECK(DxCluster); SCR_CHECK(SunSpots); SCR_CHECK(BandInfo); SCR_CHECK(Weather); SCR_CHECK(AprsList);
SCR_CHECK(Pota); SCR_CHECK(HamAlert); SCR_CHECK(AprsRadar); SCR_CHECK(WeatherOm); SCR_CHECK(ForecastOm); SCR_CHECK(Forecast); SCR_CHECK(Settings);
#undef SCR_CHECK

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
  if (id == ScreenId::WeatherOm) return createWeatherScreen(true);
  if (id == ScreenId::ForecastOm) return createForecastScreen(true);
  if (id == ScreenId::AprsRadar) return createAprsRadarScreen();
  return createClockScreen();  // unknown id
}
