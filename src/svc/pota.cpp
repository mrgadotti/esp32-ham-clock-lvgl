#include "pota.h"

#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

#include "app/config_store.h"
#include "app/data_store.h"
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
  for (const char* k : {"activator", "frequency", "mode", "reference", "spotTime", "locationDesc", "comments", "spotter"})
    filter[k] = true;  // per-element filter: the array is streamed item by item
  const char* url = c.pota_api_url[0] ? c.pota_api_url : "https://api.pota.app/v1/spots";
  Spot* spots = (Spot*)malloc(sizeof(Spot) * kMax);  // heap: static would not fit in DRAM, the stack is too small
  if (!spots) return;
  size_t n = 0;
  struct Ctx { Spot* spots; size_t* n; } ctx{spots, &n};
  bool ok = httpGetJsonArray(url, filter, [](JsonObjectConst o, void* p) {
    Ctx& c = *(Ctx*)p;
    Spot s;
    if (!potaFromJson(o, s)) return;
    if (*c.n < kMax) { c.spots[(*c.n)++] = s; return; }
    size_t oldest = 0;  // full: replace the oldest if this one is newer
    for (size_t i = 1; i < kMax; i++) if (c.spots[i].utc < c.spots[oldest].utc) oldest = i;
    if (s.utc > c.spots[oldest].utc) c.spots[oldest] = s;
  }, &ctx);
  if (!ok) {
    Serial.println("[POTA] fetch failed, keeping old list");
    free(spots);
    return;
  }
  if (n == 0) {
    Serial.println("[POTA] no spots parsed, keeping old list");
    free(spots);
    return;
  }
  potaSortByTimeDesc(spots, n);
  for (size_t i = 0; i < n; i++) qrzOnNewSpot(spots[i]);  // POTA entries carry no grid, so no distance
  g_store.pota.write([&](RingBuffer<Spot, 30>& r) {
    r.clear();
    for (size_t i = n; i-- > 0;) r.pushFront(spots[i]);
  });
  free(spots);
  Serial.printf("[POTA] %u spots\n", (unsigned)n);
}
