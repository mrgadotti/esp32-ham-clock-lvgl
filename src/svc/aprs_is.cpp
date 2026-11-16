#include "aprs_is.h"

#include <Arduino.h>
#include <WiFi.h>
#include <string.h>

#include "backoff.h"
#include "geo.h"
#include "app/config_store.h"
#include "app/data_store.h"
#include "app/service_coords.h"
#include "svc/aprs_ingest.h"

namespace {
constexpr uint32_t kSilenceMs = 300000, kFilterMinMs = 30000;
constexpr size_t kLineMax = 512, kReadBudget = 2048;

WiFiClient client;
char line[kLineMax];
size_t len;
bool discard, gotLine, up, filterSent, passOk;
uint8_t fails;
uint32_t nextTry, lastRx, lastFilterMs;
double fLat, fLon;

void drop(uint32_t now, bool failed) {
  client.stop();
  up = false;
  len = 0;
  discard = false;
  if (failed && fails < 255) fails++;
  nextTry = now + backoffMs(fails);
  filterSent = false;
}

// Login: appended to the login line. Later: "#filter" line when moved >= 1 km, at most every 30 s.
bool sendFilter(const Config& c, uint32_t now, bool inLogin) {
  double lat, lon;
  if (!serviceCoords(lat, lon)) return false;
  if (!inLogin && filterSent && (now - lastFilterMs < kFilterMinMs || distanceKm(fLat, fLon, lat, lon) < 1.f)) return false;
  client.printf("%sfilter r/%.4f/%.4f/%d", inLogin ? " " : "#", lat, lon, (int)c.aprs_radius);
  if (!inLogin) client.print("\r\n");
  fLat = lat;
  fLon = lon;
  lastFilterMs = now;
  filterSent = true;
  return true;
}
}  // namespace

bool aprsIsReadyToSend() { return up && passOk && client.connected(); }

bool aprsIsSendLine(const char* l) {
  if (!aprsIsReadyToSend()) return false;
  client.print(l);
  client.print("\r\n");
  return true;
}

void aprsIsTick(uint32_t now) {
  bool sta = false;
  g_store.net.read([&](const NetStatus& s) { sta = s.sta; });
  Config c;
  cfgGet(c);
  bool tnc = strncmp(c.uart2_cmod, "aprs_tnc", 8) == 0;  // aprs_tnc and aprs_tnc_kiss
  if (!sta || tnc || !c.aprs_callsign[0] || !c.aprs_host[0] || c.aprs_port <= 0) {
    if (up) client.stop();
    up = false;
    fails = 0;
    nextTry = 0;
    return;
  }

  if (up && !client.connected()) drop(now, true);
  if (!up) {
    if (nextTry && (int32_t)(now - nextTry) < 0) return;
    if (!client.connect(c.aprs_host, (uint16_t)c.aprs_port, 4000)) {
      Serial.println("[APRS-IS] connect failed");
      drop(now, true);
      return;
    }
    client.printf("user %s", c.aprs_callsign);
    if (c.aprs_ssid > 0) client.printf("-%d", (int)c.aprs_ssid);
    client.printf(" pass %s vers ESP32-HAM-CLOCK-LVGL 2.0", c.aprs_passcode[0] ? c.aprs_passcode : "-1");
    filterSent = false;
    sendFilter(c, now, true);
    client.print("\r\n");
    passOk = c.aprs_passcode[0] && strcmp(c.aprs_passcode, "-1") != 0;
    up = true;
    len = 0;
    discard = false;
    gotLine = false;
    lastRx = now;
    return;
  }

  size_t budget = kReadBudget;
  while (budget-- && client.available()) {
    int ch = client.read();
    if (ch < 0) break;
    lastRx = now;
    if (ch == '\r') continue;
    if (ch == '\n') {
      line[len] = 0;
      if (len && !discard && line[0] != '#') {
        if (!gotLine) { gotLine = true; fails = 0; }
        aprsIngestLine(line, 'I');
      }
      len = 0;
      discard = false;
    } else if (len < kLineMax - 1) {
      line[len++] = (char)ch;
    } else {
      discard = true;
    }
  }
  if (now - lastRx > kSilenceMs) {
    Serial.println("[APRS-IS] silence watchdog");
    drop(now, true);
    return;
  }
  sendFilter(c, now, false);
}
