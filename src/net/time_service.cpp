#include "time_service.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_sntp.h>

#include "app/config_store.h"
#include "svc/uart2.h"

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

static volatile uint32_t g_ntpSyncMs;  // millis() of the last SNTP sync, 0 = never
bool ntpSyncedWithin(uint32_t ms) {
  uint32_t t = g_ntpSyncMs;
  return t && millis() - t < ms;
}

void timeTick(uint32_t now) {
  static uint32_t last = 0;
  static bool started = false;
  if (WiFi.status() != WL_CONNECTED) return;
  if (gpsTimeValid()) return;
  if (!started || now - last >= 3600000UL) {
    if (!started) sntp_set_time_sync_notification_cb([](struct timeval*) { g_ntpSyncMs = millis() | 1; });
    configTime(0, 0, "pool.ntp.org", "time.google.com");  // re-calling re-syncs
    started = true;
    last = now;
  }
}
