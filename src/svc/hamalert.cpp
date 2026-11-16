#include "hamalert.h"

#include <Arduino.h>
#include <WiFi.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "app/config_store.h"
#include "app/data_store.h"
#include "app/service_coords.h"
#include "geo.h"
#include "hamalert_parse.h"
#include "net/time_service.h"
#include "svc/qrz.h"

namespace {
constexpr uint32_t kPeriodMs = 60000, kTotalMs = 9000, kIdleMs = 1800, kPreLoginMs = 1200;
constexpr size_t kMax = 30, kLineMax = 1400;
struct Work { Spot spots[kMax]; char line[kLineMax + 1]; };  // heap per poll: static would not fit in DRAM
}  // namespace

void hamalertTick(uint32_t now) {
  static uint32_t last;
  static bool tried;
  bool sta = false;
  g_store.net.read([&](const NetStatus& s) { sta = s.sta; });
  Config c;
  cfgGet(c);
  if (!sta || !c.hama_host[0] || c.hama_port <= 0 || !c.hama_user[0] || !c.hama_pass[0] || !timeValid()) return;
  if (tried && now - last < kPeriodMs) return;
  tried = true;
  last = now;

  Work* w = (Work*)malloc(sizeof(Work));
  if (!w) return;
  Spot* spots = w->spots;
  char* line = w->line;
  WiFiClient client;
  client.setTimeout(1200);
  if (!client.connect(c.hama_host, (uint16_t)c.hama_port, 4000)) {
    Serial.println("[HAMALERT] connect failed, keeping old list");
    free(w);
    return;
  }
  uint32_t t0 = millis();
  while (millis() - t0 < kPreLoginMs && client.connected()) {  // swallow banner/prompt
    while (client.available()) client.read();
    delay(2);
  }
  client.print(c.hama_user); client.print("\n");
  delay(180);
  client.print(c.hama_pass); client.print("\n");  // never logged
  delay(220);
  client.print("set/json\n");
  delay(160);
  client.print("sh/dx 30\n");

  size_t n = 0, len = 0;
  bool discard = false;
  uint32_t nowUtc = (uint32_t)time(nullptr);
  uint32_t start = millis(), lastRx = start;
  while (client.connected() && millis() - start < kTotalMs && n < kMax) {
    while (client.available() && n < kMax && millis() - start < kTotalMs) {
      int ch = client.read();
      if (ch < 0) break;
      lastRx = millis();
      if (ch == '\r') continue;
      if (ch == '\n') {
        line[len] = 0;
        if (len && !discard && hamalertParseLine(line, nowUtc, spots[n])) n++;
        len = 0;
        discard = false;
      } else if (len < kLineMax) {
        line[len++] = (char)ch;
      } else {
        discard = true;
      }
    }
    if (n > 0 && millis() - lastRx > kIdleMs) break;
    delay(2);
  }
  client.stop();
  if (n == 0) {
    Serial.println("[HAMALERT] no spots, keeping old list");
    free(w);
    return;
  }

  double ml, mo;
  bool haveMe = serviceCoords(ml, mo);
  for (size_t i = 0; i < n; i++) {
    double la, lo;
    if (spots[i].loc[0] && locatorToLatLon(spots[i].loc, la, lo)) {
      spots[i].lat = (float)la; spots[i].lon = (float)lo; spots[i].hasLatLon = true;
      if (haveMe) spots[i].distKm = (int16_t)distanceKm(ml, mo, la, lo);
    }
    qrzOnNewSpot(spots[i]);
  }
  g_store.hamalert.write([&](RingBuffer<Spot, 30>& r) {
    r.clear();
    for (size_t i = n; i-- > 0;) r.pushFront(spots[i]);  // server sends newest first
  });
  free(w);
  Serial.printf("[HAMALERT] %u spots\n", (unsigned)n);
}
