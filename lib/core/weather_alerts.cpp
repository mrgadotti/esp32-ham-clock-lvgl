#include "weather_alerts.h"
#include <string.h>

static const uint32_t CURRENT_MAX_AGE_MS = 30UL * 60UL * 1000UL;
static const uint32_t WIND_REPEAT_MS = 60UL * 1000UL;

static uint8_t repeatMinutesFromEta(int etaMin) {
  if (etaMin <= 0) return 1;
  int h = (etaMin + 59) / 60;
  return (uint8_t)(h < 1 ? 1 : h > 6 ? 6 : h);
}

static bool reached(uint32_t now, uint32_t at) { return at == 0 || (int32_t)(now - at) >= 0; }

bool WeatherAlertScheduler::due(const WeatherData& w, uint32_t now, const WxAlertCfg& cfg, WeatherAlertInfo& out) {
  memset(&out, 0, sizeof out);
  const bool fcFresh = w.forecastFetchedAt != 0;
  const bool stormNow = cfg.storm && w.valid && w.weatherId >= 200 && w.weatherId < 300 &&
                        w.fetchedAt != 0 && (uint32_t)(now - w.fetchedAt) <= CURRENT_MAX_AGE_MS;
  const bool stormFc = cfg.storm && fcFresh && w.stormEtaMin > 0;
  const bool stormActive = stormNow || stormFc;
  const bool windActive = cfg.wind && fcFresh &&
      (w.fcWindMaxMs >= cfg.windThresholdMs || w.fcWindGustMs >= cfg.windThresholdMs + 5.0f);
  bool notify = false;

  if (stormActive != stormPrev_ || !stormActive) nextStorm_ = 0;
  stormPrev_ = stormActive;
  if (windActive != windPrev_ || !windActive) nextWind_ = 0;
  windPrev_ = windActive;

  if (stormActive && reached(now, nextStorm_)) {
    int eta = w.stormEtaMin - (fcFresh && (int32_t)(now - w.forecastFetchedAt) > 0 ? (int)((uint32_t)(now - w.forecastFetchedAt) / 60000UL) : 0);
    if (eta < 1) eta = 1;
    uint8_t rep = stormNow ? 1 : repeatMinutesFromEta(eta);
    out.stormPossible = true; out.stormNow = stormNow; out.stormEtaHours = rep;  // field carries minutes
    nextStorm_ = now + (uint32_t)rep * 60000UL;
    if (nextStorm_ == 0) nextStorm_ = 1;
    notify = true;
  }
  if (windActive && reached(now, nextWind_)) {
    out.windPossible = true;
    out.maxWindMs = w.fcWindMaxMs > w.windMs ? w.fcWindMaxMs : w.windMs;
    out.maxWindGustMs = w.fcWindGustMs;
    nextWind_ = now + WIND_REPEAT_MS;
    if (nextWind_ == 0) nextWind_ = 1;
    notify = true;
  }
  return notify;
}
