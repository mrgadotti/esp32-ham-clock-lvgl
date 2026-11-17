#include "pota.h"

#include <Arduino.h>
#include <string.h>

#include "app/config_store.h"
#include "app/data_store.h"
#include "net/http_fetch.h"
#include "pota_parse.h"
#include "svc/qrz.h"
#include "svc/spot_scratch.h"

namespace {
constexpr uint32_t kPeriodMs = 180000;
constexpr size_t kMax = kSpotScratchN;
}  // namespace

void potaTick(uint32_t now) {
  static uint32_t last;
  static bool tried;
  bool sta = false;
  g_store.net.read([&](const NetStatus& n) { sta = n.sta; });
  if (!sta || (tried && now - last < kPeriodMs) || !httpPollGate(now)) return;
  tried = true;
  last = now;

  char apiUrl[sizeof(Config::pota_api_url)];
  cfgRead([&](const Config& c) { strlcpy(apiUrl, c.pota_api_url, sizeof apiUrl); });
  JsonDocument filter;
  for (const char* k : {"activator", "frequency", "mode", "reference", "spotTime", "locationDesc", "comments"})
    filter[k] = true;  // per-element filter: the array is streamed item by item
  const char* url = apiUrl[0] ? apiUrl : "https://api.pota.app/v1/spots";
  Spot* spots = spotScratch();
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
    uint32_t q = httpQuickRetryMs();
    if (q) last = now + q - kPeriodMs;  // one quick retry after an SSL allocation failure
    return;
  }
  if (n == 0) {
    Serial.println("[POTA] no spots parsed, keeping old list");
    return;
  }
  potaSortByTimeDesc(spots, n);
  for (size_t i = 0; i < n; i++) qrzOnNewSpot(spots[i]);  // POTA entries carry no grid, so no distance
  g_store.pota.write([&](RingBuffer<Spot, kSpotScratchN>& r) {
    r.clear();
    for (size_t i = n; i-- > 0;) r.pushFront(spots[i]);
  });
  Serial.printf("[POTA] %u spots\n", (unsigned)n);
}
