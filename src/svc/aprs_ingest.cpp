#include "aprs_ingest.h"

#include <Arduino.h>
#include <freertos/semphr.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include "alert_rules.h"
#include "aprs_message.h"
#include "aprs_parse.h"
#include "geo.h"
#include "app/config_store.h"
#include "app/data_store.h"
#include "app/events.h"
#include "app/service_coords.h"
#include "hal/rgb_led.h"
#include "net/time_service.h"
#include "svc/aprs_beacon.h"

namespace {
AlertRules rules;
SemaphoreHandle_t mu;  // guards rules + the cached config below
AlertCfg lastCfg;
bool ledEn, cfgSeen;
uint32_t ledDur, ledBlk, lastVer;
char myCall[16];  // CALL or CALL-SSID, guarded by mu

// Caller holds mu. Re-read the config only when cfgVersion() moves, not per line.
void refreshRules(uint32_t) {
  uint32_t ver = cfgVersion();  // read before the config: a racing update only costs one extra refresh
  if (cfgSeen && ver == lastVer) return;
  lastVer = ver;
  AlertCfg a{};
  cfgRead([&](const Config& c) {
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
    if (c.aprs_ssid > 0) snprintf(myCall, sizeof myCall, "%s-%d", c.aprs_callsign, (int)c.aprs_ssid);
    else strlcpy(myCall, c.aprs_callsign, sizeof myCall);
  });
  if (cfgSeen && memcmp(&a, &lastCfg, sizeof a) == 0) return;
  lastCfg = a;
  cfgSeen = true;
  rules.configure(a);
}

// True when the line was a message for us (new or duplicate): the caller stops there.
bool ingestMessage(const char* line) {
  char me[sizeof myCall];
  xSemaphoreTake(mu, portMAX_DELAY);
  refreshRules(millis());
  memcpy(me, myCall, sizeof me);
  xSemaphoreGive(mu);
  AprsMessage m{};
  if (!me[0] || !aprsParseMessage(line, me, m)) return false;
  if (m.msgNo[0]) aprsAckRequest(m.from, m.msgNo);  // every copy, retries included, so the sender stops
  m.utc = timeValid() ? (uint32_t)time(nullptr) : 0;
  g_store.aprs.read([&](const AprsRing& r) {
    for (size_t i = 0; i < r.size(); i++) {
      const AprsStation& a = r.at(i);
      if (strcmp(a.call, m.from) != 0) continue;
      m.sym[0] = a.sym[0];
      m.sym[1] = a.sym[1];
      m.hasLatLon = a.hasLatLon;
      m.distKm = a.distKm;
      break;
    }
  });
  bool dup = false;
  g_store.aprsMsg.write([&](auto& r) {
    for (size_t i = 0; i < r.size() && !dup; i++) dup = aprsMsgSame(r.at(i), m);
    if (!dup) r.pushFront(m);
  });
  if (dup) return true;
  UiEvent e{};
  e.kind = EventKind::AprsMessage;
  e.msg = m;
  postUiEvent(e);
  return true;
}
}  // namespace

void aprsIngestInit() { mu = xSemaphoreCreateMutex(); }

void aprsIngestLine(const char* line, char source) {
  if (ingestMessage(line)) return;
  AprsStation st{};
  if (!aprsParseFrame(line, st)) return;
  st.source = source;
  st.utc = timeValid() ? (uint32_t)time(nullptr) : 0;
  double lat, lon;
  if (st.hasLatLon && serviceCoords(lat, lon)) {
    st.distKm = (int16_t)fminf(lroundf(distanceKm(lat, lon, st.lat, st.lon)), 32000.f);
    st.bearing = (int16_t)(lroundf(bearingDeg(lat, lon, st.lat, st.lon)) % 360);
  }
  g_store.aprs.write([&](AprsRing& r) {
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
