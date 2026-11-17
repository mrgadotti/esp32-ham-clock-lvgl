#include "aprs_is.h"

#include <Arduino.h>
#include <WiFi.h>
#include <string.h>

#include "backoff.h"
#include "geo.h"
#include "app/config_store.h"
#include "app/data_store.h"
#include "app/service_coords.h"
#include "app/mem_log.h"
#include "svc/aprs_ingest.h"
#include "svc/async_connect.h"
#include "svc/line_reader.h"

namespace {
constexpr uint32_t kSilenceMs = 300000, kFilterMinMs = 30000;
constexpr size_t kLineMax = 512, kReadBudget = 2048;

struct Cfg {
  char host[64], call[12], pass[8], uart2Cmod[20];
  int32_t port, ssid, radius;
} c;  // net task only; refreshed when cfgVersion() moves
uint32_t cfgVer;
bool haveCfg;

Cfg used;  // settings of the session in progress: a change reconnects
AsyncConnect conn;
WiFiClient client;
char line[kLineMax];
LineReader reader(line, sizeof line, false);
bool gotLine, up, connecting, filterSent, passOk;
uint8_t fails;
uint32_t nextTry, lastRx, lastFilterMs;
double fLat, fLon;

void refreshCfg() {
  uint32_t v = cfgVersion();
  if (haveCfg && v == cfgVer) return;
  haveCfg = true;
  cfgVer = v;
  cfgRead([](const Config& s) {
    strlcpy(c.host, s.aprs_host, sizeof c.host);
    strlcpy(c.call, s.aprs_callsign, sizeof c.call);
    strlcpy(c.pass, s.aprs_passcode, sizeof c.pass);
    strlcpy(c.uart2Cmod, s.uart2_cmod, sizeof c.uart2Cmod);
    c.port = s.aprs_port; c.ssid = s.aprs_ssid; c.radius = s.aprs_radius;
  });
}

bool sameSession(const Cfg& a, const Cfg& b) {
  return a.port == b.port && a.ssid == b.ssid && a.radius == b.radius && !strcmp(a.host, b.host) && !strcmp(a.call, b.call) &&
         !strcmp(a.pass, b.pass);
}

void drop(uint32_t now, bool failed) {
  conn.cancel();
  client.stop();
  up = connecting = false;
  reader.reset();
  if (failed && fails < 255) fails++;
  nextTry = now + backoffMs(fails);
  filterSent = false;
}

// Login: appended to the login line. Later: "#filter" line when moved >= 1 km, at most every 30 s.
bool sendFilter(uint32_t now, bool inLogin) {
  double lat, lon;
  if (!serviceCoords(lat, lon)) return false;
  if (!inLogin && filterSent && (now - lastFilterMs < kFilterMinMs || distanceKm(fLat, fLon, lat, lon) < 1.f)) return false;
  client.printf("%sfilter r/%.4f/%.4f/%d", inLogin ? " " : "#", lat, lon, (int)c.radius);
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
  char buf[192];
  int n = snprintf(buf, sizeof buf, "%s\r\n", l);
  return n > 0 && n < (int)sizeof buf && client.write((const uint8_t*)buf, n) == (size_t)n;
}

void aprsIsTick(uint32_t now) {
  bool sta = false;
  g_store.net.read([&](const NetStatus& s) { sta = s.sta; });
  refreshCfg();
  bool tnc = strncmp(c.uart2Cmod, "aprs_tnc", 8) == 0;  // aprs_tnc and aprs_tnc_kiss
  if (!sta || tnc || !c.call[0] || !c.host[0] || c.port <= 0) {
    conn.cancel();
    client.stop();
    up = connecting = false;
    fails = 0;
    nextTry = 0;
    return;
  }

  if ((up || connecting) && !sameSession(used, c)) {
    drop(now, false);
    fails = 0;
    nextTry = 0;
  }
  if (up && !client.connected()) drop(now, true);
  if (connecting) {
    AsyncConnect::State st = conn.poll(now, client);
    if (st == AsyncConnect::Dns || st == AsyncConnect::Connecting) return;
    connecting = false;
    if (st != AsyncConnect::Done) {
      Serial.println("[APRS-IS] connect failed");
      drop(now, true);
      return;
    }
    MEM_DIAG_ONCE("aprs-is: after connect");
    client.printf("user %s", c.call);
    if (c.ssid > 0) client.printf("-%d", (int)c.ssid);
    client.printf(" pass %s vers ESP32-HAM-CLOCK-LVGL 2.0", c.pass[0] ? c.pass : "-1");
    filterSent = false;
    sendFilter(now, true);
    client.print("\r\n");
    passOk = c.pass[0] && strcmp(c.pass, "-1") != 0;
    up = true;
    reader.reset();
    gotLine = false;
    lastRx = now;
    return;
  }
  if (!up) {
    if (nextTry && (int32_t)(now - nextTry) < 0) return;
    MEM_DIAG_ONCE("aprs-is: before connect");
    used = c;
    conn.start(c.host, (uint16_t)c.port, now);
    connecting = true;
    return;
  }

  if (reader.pump(client, kReadBudget, [](const char* l) {
        if (l[0] == '#') return;
        if (!gotLine) { gotLine = true; fails = 0; }
        aprsIngestLine(l, 'I');
      }))
    lastRx = now;
  if (now - lastRx > kSilenceMs) {
    Serial.println("[APRS-IS] silence watchdog");
    drop(now, true);
    return;
  }
  sendFilter(now, false);
}
