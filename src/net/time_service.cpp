#include "time_service.h"
#include "app/boot_log.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_sntp.h>

#include "svc/uart2.h"

bool timeValid() {
  time_t t = time(nullptr);
  struct tm tm;
  gmtime_r(&t, &tm);
  return tm.tm_year + 1900 >= 2024;
}

static volatile uint32_t g_ntpSyncMs;  // millis() of the last SNTP sync, 0 = never
bool ntpSyncedWithin(uint32_t ms) {
  uint32_t t = g_ntpSyncMs;
  return t && millis() - t < ms;
}

void timeTick(uint32_t now) {
  static bool logged;
  if (!logged && g_ntpSyncMs) {  // logged here, not in the SNTP callback (tcpip task, small stack)
    logged = true;
    time_t t = time(nullptr);
    struct tm tm;
    gmtime_r(&t, &tm);
    bootLog("NTP synced %04d-%02d-%02d %02d:%02d UTC", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min);
  }
  static uint32_t last = 0;
  static bool started = false;
  if (WiFi.status() != WL_CONNECTED) return;
  if (gpsTimeValid()) return;
  if (!started || now - last >= 3600000UL) {
    if (!started) sntp_set_time_sync_notification_cb([](struct timeval*) { g_ntpSyncMs = millis() | 1; });
    configTime(0, 0, "pool.ntp.org", "time.google.com");  // re-calling re-syncs
    if (!started) bootLog("NTP waiting for sync...");
    started = true;
    last = now;
  }
}
