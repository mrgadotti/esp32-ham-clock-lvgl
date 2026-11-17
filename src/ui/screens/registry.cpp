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
#include "location_screen.h"

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
  if (id == ScreenId::AprsRadar) return createAprsRadarScreen();
  if (id == ScreenId::Location) return createLocationScreen();
  return createClockScreen();  // reserved ids (Matrix/Game) never have a screen
}
