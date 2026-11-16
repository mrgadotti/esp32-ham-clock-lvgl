#include "time_service.h"

#include <Arduino.h>
#include <WiFi.h>

#include "app/config_store.h"
#include "app/data_store.h"

bool timeValid() {
  time_t t = time(nullptr);
  struct tm tm;
  gmtime_r(&t, &tm);
  return tm.tm_year + 1900 >= 2024;
}

void localTimeOf(time_t utc, struct tm& out) {
  Config c;
  cfgGet(c);
  utc += (time_t)(c.timezone * 3600.0f);
  gmtime_r(&utc, &out);
}

void timeTick(uint32_t now) {
  static uint32_t last = 0;
  static bool started = false;
  if (WiFi.status() != WL_CONNECTED) return;
  bool gpsTime = false;
  g_store.gps.read([&](const GpsState& g) { gpsTime = g.active && g.fix && g.utc != 0; });
  if (gpsTime) return;
  if (!started || now - last >= 3600000UL) {
    configTime(0, 0, "pool.ntp.org", "time.google.com");  // re-calling re-syncs
    started = true;
    last = now;
  }
}
