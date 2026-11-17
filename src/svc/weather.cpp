#include "svc/weather.h"

#include <Arduino.h>
#include <WiFi.h>
#include <string.h>
#include <time.h>

#include "app/config_store.h"
#include "app/data_store.h"
#include "app/events.h"
#include "app/service_coords.h"
#include "net/http_fetch.h"
#include "net/time_service.h"
#include "weather_alerts.h"
#include "weather_json.h"

constexpr uint32_t CUR_MS = 10UL * 60000UL, FC_MS = 30UL * 60000UL, RETRY_MS = 2UL * 60000UL;

static WeatherAlertScheduler s_sched;
static Config s_cfg;  // static: keeps it off the net task stack
static WeatherData s_w;
static uint32_t s_nextCur, s_nextFc, s_lastSched;
static bool s_curFirst = true, s_fcFirst = true;  // first fetch fires immediately

static uint32_t retryMs() {
  uint32_t q = httpQuickRetryMs();  // one quick retry after an SSL allocation failure
  return q ? q : RETRY_MS;
}

static bool reached(uint32_t now, uint32_t at, bool first) { return first || (int32_t)(now - at) >= 0; }

// The URL carries the API key: never log it.
static void url(char* out, size_t n, const char* ep, double lat, double lon, const char* extra) {
  snprintf(out, n, "https://api.openweathermap.org/data/2.5/%s?lat=%.4f&lon=%.4f&appid=%s%s", ep, lat, lon,
           s_cfg.weather_key, extra);
}

static void fetchCurrent(uint32_t now, double lat, double lon) {
  char u[220], extra[40];
  snprintf(extra, sizeof extra, "&units=metric&lang=%s", s_cfg.tft_lang == 0 ? "en" : "pt_br");
  url(u, sizeof u, "weather", lat, lon, extra);
  JsonDocument f;
  f["cod"] = true;
  f["name"] = true;
  f["weather"][0]["id"] = true;
  f["weather"][0]["description"] = true;
  f["weather"][0]["icon"] = true;
  f["main"]["temp"] = true;
  f["main"]["humidity"] = true;
  f["main"]["pressure"] = true;
  f["wind"]["speed"] = true;
  JsonDocument doc;
  g_store.weather.read([](const WeatherData& w) { s_w = w; });
  bool ok = httpGetJson(u, doc, &f) && weatherParseCurrent(doc, s_w);
  if (ok) {
    s_w.fetchedAt = now | 1;
    JsonDocument af, ad;
    af["list"][0]["components"]["pm2_5"] = true;
    af["list"][0]["components"]["pm10"] = true;
    url(u, sizeof u, "air_pollution", lat, lon, "");
    if (httpGetJson(u, ad, &af)) weatherParseAir(ad, s_w);
  }
  s_w.fetchFailed = !ok;
  g_store.weather.write([](WeatherData& w) { w = s_w; });
  s_curFirst = false;
  s_nextCur = now + (ok ? CUR_MS : retryMs());
}

static void fetchForecast(uint32_t now, double lat, double lon) {
  char u[220], extra[40];
  snprintf(extra, sizeof extra, "&units=metric&lang=%s&cnt=24", s_cfg.tft_lang == 0 ? "en" : "pt_br");
  url(u, sizeof u, "forecast", lat, lon, extra);
  JsonDocument f;
  f["cod"] = true;
  f["city"]["name"] = true;
  f["list"][0]["dt"] = true;
  f["list"][0]["main"]["temp"] = true;
  f["list"][0]["main"]["humidity"] = true;
  f["list"][0]["wind"]["speed"] = true;
  f["list"][0]["wind"]["gust"] = true;
  f["list"][0]["weather"][0]["id"] = true;
  f["list"][0]["weather"][0]["icon"] = true;
  JsonDocument doc;
  g_store.weather.read([](const WeatherData& w) { s_w = w; });
  long t = timeValid() ? (long)time(nullptr) : 0;
  bool ok = httpGetJson(u, doc, &f) && weatherParseForecast(doc, s_w, t, (long)(s_cfg.timezone * 3600.0f));
  if (ok) s_w.forecastFetchedAt = now | 1;
  s_w.forecastFetchFailed = !ok;
  g_store.weather.write([](WeatherData& w) { w = s_w; });
  s_fcFirst = false;
  s_nextFc = now + (ok ? FC_MS : retryMs());
}

void weatherTick(uint32_t now) {
  if (now - s_lastSched >= 1000) {  // alerts keep running on cached data
    s_lastSched = now;
    cfgGet(s_cfg);
    g_store.weather.read([](const WeatherData& w) { s_w = w; });
    WxAlertCfg ac{s_cfg.wx_al_st, s_cfg.wx_al_wi, s_cfg.wx_w_thr};
    UiEvent e{};
    if (s_w.valid && s_sched.due(s_w, now, ac, e.wx)) {
      e.kind = EventKind::WeatherAlert;
      postUiEvent(e);
    }
  }
  if (WiFi.status() != WL_CONNECTED) return;
  bool cd = reached(now, s_nextCur, s_curFirst), fd = reached(now, s_nextFc, s_fcFirst);
  if (!cd && !fd) return;
  cfgGet(s_cfg);
  double lat, lon;
  if (!s_cfg.weather_key[0] || !serviceCoords(lat, lon)) return;
  if (!httpPollGate(now)) return;
  if (cd) fetchCurrent(now, lat, lon);
  else fetchForecast(now, lat, lon);
}
