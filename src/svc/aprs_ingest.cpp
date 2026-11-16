#include "aprs_ingest.h"

#include <Arduino.h>
#include <freertos/semphr.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include "alert_rules.h"
#include "aprs_parse.h"
#include "geo.h"
#include "app/config_store.h"
#include "app/data_store.h"
#include "app/events.h"
#include "app/service_coords.h"
#include "hal/rgb_led.h"
#include "net/time_service.h"

namespace {
AlertRules rules;
SemaphoreHandle_t mu;  // guards rules + the cached config below
AlertCfg lastCfg;
bool ledEn, cfgSeen;
uint32_t ledDur, ledBlk, lastRead;

// Caller holds mu. Config has no change sequence, so re-read at most every 2 s instead of copying it per line.
void refreshRules(uint32_t now) {
  if (cfgSeen && now - lastRead < 2000) return;
  lastRead = now;
  Config c;
  cfgGet(c);
  AlertCfg a{};
  strncpy(a.watchList, c.aprs_alert, sizeof a.watchList - 1);
  a.alertEnabled = c.aprs_alert_en;
  a.rangeEnabled = c.aprs_al_near;
  a.wxEnabled = c.alrt_wx_en;
  a.ledEnabled = c.led_al_en;
  a.alertKm = c.alrt_dst_km;
  a.minSeconds = (uint16_t)c.aprs_alrt_sec;
  ledEn = c.led_al_en;
  ledDur = (uint32_t)c.led_al_dur;
  ledBlk = (uint32_t)c.led_al_blk;
  if (cfgSeen && memcmp(&a, &lastCfg, sizeof a) == 0) return;
  lastCfg = a;
  cfgSeen = true;
  rules.configure(a);
}
}  // namespace

void aprsIngestInit() { mu = xSemaphoreCreateMutex(); }

void aprsIngestLine(const char* line, char source) {
  AprsStation st{};
  if (!aprsParseFrame(line, st)) return;
  st.source = source;
  st.utc = timeValid() ? (uint32_t)time(nullptr) : 0;
  double lat, lon;
  if (st.hasLatLon && serviceCoords(lat, lon)) {
    st.distKm = (int16_t)fminf(lroundf(distanceKm(lat, lon, st.lat, st.lon)), 32000.f);
    st.bearing = (int16_t)(lroundf(bearingDeg(lat, lon, st.lat, st.lon)) % 360);
  }
  g_store.aprs.write([&](RingBuffer<AprsStation, 30>& r) {
    if (!r.moveToFrontIf([&](const AprsStation& a) { return strcmp(a.call, st.call) == 0; }, st)) r.pushFront(st);
  });

  uint32_t now = millis(), dur, blk;
  bool hit, led;
  xSemaphoreTake(mu, portMAX_DELAY);
  refreshRules(now);
  hit = rules.shouldAlert(st, now);
  led = ledEn;
  dur = ledDur;
  blk = ledBlk;
  xSemaphoreGive(mu);
  if (!hit) return;
  UiEvent e{};
  e.kind = EventKind::AprsAlert;
  e.station = st;
  postUiEvent(e);
  if (led && !st.isWx) rgbLedBlink(RGB_BLUE, dur, blk);
}
