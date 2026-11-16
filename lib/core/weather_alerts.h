#pragma once
#include "types.h"
// Decides when a weather alert should be shown. Time is injected (ms, wrap-safe).
class WeatherAlertScheduler {
public:
  // Returns true and fills `out` when an alert is due. out.stormEtaHours carries the repeat interval in MINUTES.
  bool due(const WeatherData& w, uint32_t nowMs, const WxAlertCfg& cfg, WeatherAlertInfo& out);
private:
  uint32_t nextStorm_ = 0, nextWind_ = 0;  // 0 = fire immediately
  bool stormPrev_ = false, windPrev_ = false;
};
