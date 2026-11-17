#include "pota.h"
#include "app/boot_log.h"

#include <Arduino.h>
#include <string.h>

#include "app/config_store.h"
#include "app/data_store.h"
#include "net/http_fetch.h"
#include "backoff.h"
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
  static uint8_t fails;
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
    g_store.net.write([](NetStatus& s) { s.potaFail = true; });
    bool empty = true;
    g_store.pota.read([&](const RingBuffer<Spot, 30>& r) { empty = r.size() == 0; });
    uint32_t q = httpQuickRetryMs();  // one quick retry after an SSL allocation failure
    uint32_t wait = q ? q : empty ? listRetryMs(fails) : kPeriodMs;  // empty list: retry sooner than the normal period
    if (fails < 255) fails++;
    last = now + wait - kPeriodMs;
    return;
  }
  fails = 0;
  bool wasFail = false;
  g_store.net.read([&](const NetStatus& s) { wasFail = s.potaFail; });
  if (wasFail) g_store.net.write([](NetStatus& s) { s.potaFail = false; });
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
  static bool logged;
  if (!logged) { logged = true; bootLog("POTA ok, %u spots", (unsigned)n); }
}
