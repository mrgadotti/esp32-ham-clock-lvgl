#include "pota.h"

#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

#include "app/config_store.h"
#include "app/data_store.h"
#include "app/service_coords.h"
#include "geo.h"
#include "net/http_fetch.h"
#include "pota_parse.h"
#include "svc/qrz.h"

namespace {
constexpr uint32_t kPeriodMs = 180000;
constexpr size_t kMax = 30;
}  // namespace

void potaTick(uint32_t now) {
  static uint32_t last;
  static bool tried;
  bool sta = false;
  g_store.net.read([&](const NetStatus& n) { sta = n.sta; });
  if (!sta || (tried && now - last < kPeriodMs)) return;
  tried = true;
  last = now;

  Config c;
  cfgGet(c);
  JsonDocument filter;
  JsonObject f = filter.add<JsonObject>();
  for (const char* k : {"activator", "frequency", "mode", "reference", "spotTime", "locationDesc", "comments", "spotter"})
    f[k] = true;
  JsonDocument doc;
  if (!httpGetJson(c.pota_api_url[0] ? c.pota_api_url : "https://api.pota.app/v1/spots", doc, &filter) ||
      !doc.is<JsonArray>()) {
    Serial.println("[POTA] fetch failed, keeping old list");
    return;
  }

  Spot* spots = (Spot*)malloc(sizeof(Spot) * kMax);  // heap: static would not fit in DRAM, the stack is too small
  if (!spots) return;
  size_t n = 0;
  for (JsonObjectConst o : doc.as<JsonArrayConst>()) {
    Spot s;
    if (!potaFromJson(o, s)) continue;
    if (n < kMax) { spots[n++] = s; continue; }
    size_t oldest = 0;  // full: replace the oldest if this one is newer
    for (size_t i = 1; i < kMax; i++) if (spots[i].utc < spots[oldest].utc) oldest = i;
    if (s.utc > spots[oldest].utc) spots[oldest] = s;
  }
  potaSortByTimeDesc(spots, n);

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
  g_store.pota.write([&](RingBuffer<Spot, 30>& r) {
    r.clear();
    for (size_t i = n; i-- > 0;) r.pushFront(spots[i]);
  });
  free(spots);
  Serial.printf("[POTA] %u spots\n", (unsigned)n);
}
