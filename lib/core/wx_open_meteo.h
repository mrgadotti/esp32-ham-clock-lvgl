#pragma once
#include <ArduinoJson.h>
#include "i18n.h"
#include "types.h"
// Open-Meteo forecast / air-quality parsing (pure; no network).
constexpr const char* OM_FORECAST_QUERY =
  "current=temperature_2m,relative_humidity_2m,surface_pressure,wind_speed_10m,weather_code,is_day"
  "&hourly=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m,wind_gusts_10m"
  "&daily=weather_code,temperature_2m_max,temperature_2m_min,relative_humidity_2m_mean,wind_speed_10m_max"
  "&forecast_hours=7&forecast_days=3&timezone=auto&wind_speed_unit=ms";

int wmoToOwmId(int wmo);   // WMO code -> OpenWeather id (family-exact); unknown -> 800
Tr wxOmDescTr(int owmId);  // condition text for an OpenWeather id; WxCondUnknown when unmapped
// Fills current + detail columns 0..3 (hourly 3, 6; daily 1, 2) + nightTempC[0..1] + fcWind*.
// airOnly: reads only current.pm2_5/pm10. Returns false and leaves `out` untouched on bad bodies.
bool openMeteoParse(JsonDocument& doc, WeatherData& out, bool airOnly = false);
