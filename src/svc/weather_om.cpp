#include "svc/weather_om.h"

#include <Arduino.h>
#include <WiFi.h>

#include "app/config_store.h"
#include "app/data_store.h"
#include "app/service_coords.h"
#include "net/http_fetch.h"
#include "wx_open_meteo.h"

constexpr uint32_t OM_MS = 10UL * 60000UL, OM_RETRY_MS = 5UL * 60000UL;
constexpr int AIR_EVERY = 6;

static Config s_cfg;  // static: keeps it off the net task stack
static WeatherData s_w;
static uint32_t s_next, s_cycles;
static bool s_first = true;  // first fetch fires immediately

static bool reached(uint32_t now, uint32_t at, bool first) { return first || (int32_t)(now - at) >= 0; }

static void fetch(uint32_t now, double lat, double lon) {
  char u[400];  // the URL carries the coordinates: never log it
  snprintf(u, sizeof u, "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f&%s", lat, lon,
           OM_FORECAST_QUERY);
  JsonDocument f;
  f["error"] = true;
  for (const char* k : {"temperature_2m", "relative_humidity_2m", "surface_pressure", "wind_speed_10m", "weather_code", "is_day"})
    f["current"][k] = true;
  for (const char* k : {"temperature_2m", "relative_humidity_2m", "weather_code", "wind_speed_10m", "wind_gusts_10m"})
    f["hourly"][k] = true;
  for (const char* k : {"weather_code", "temperature_2m_max", "temperature_2m_min", "relative_humidity_2m_mean", "wind_speed_10m_max"})
    f["daily"][k] = true;
  JsonDocument doc;
  g_store.weatherOm.read([](const WeatherData& w) { s_w = w; });
  bool ok = httpGetJson(u, doc, &f) && openMeteoParse(doc, s_w);
  if (ok) {
    s_w.fetchedAt = s_w.forecastFetchedAt = now | 1;
    if (s_cycles++ % AIR_EVERY == 0) {  // a failed air call never fails the cycle
      JsonDocument af, ad;
      af["error"] = true;
      af["current"]["pm2_5"] = true;
      af["current"]["pm10"] = true;
      snprintf(u, sizeof u, "https://air-quality-api.open-meteo.com/v1/air-quality?latitude=%.4f&longitude=%.4f&current=pm2_5,pm10", lat, lon);
      if (httpGetJson(u, ad, &af)) openMeteoParse(ad, s_w, true);
    }
  }
  s_w.fetchFailed = s_w.forecastFetchFailed = !ok;
  g_store.weatherOm.write([](WeatherData& w) { w = s_w; });
  s_first = false;
  uint32_t q = httpQuickRetryMs();
  s_next = now + (ok ? OM_MS : (q ? q : OM_RETRY_MS));
}

void weatherOmTick(uint32_t now) {
  cfgGet(s_cfg);
  if (!(s_cfg.scr_en & ((1 << 10) | (1 << 11)))) return;
  if (WiFi.status() != WL_CONNECTED || !reached(now, s_next, s_first) || !httpPollGate(now)) return;
  double lat, lon;
  if (serviceCoords(lat, lon)) fetch(now, lat, lon);
}
