#include "aprs_ingest.h"

#include <Arduino.h>
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
AlertCfg lastCfg;
bool cfgSeen;

// shortcut: no mutex around AlertRules; the only racing callers are APRS-IS and UART2, a lost cooldown slot is harmless.
void refreshRules(const Config& c) {
  AlertCfg a{};
  strncpy(a.watchList, c.aprs_alert, sizeof a.watchList - 1);
  a.alertEnabled = c.aprs_alert_en;
  a.rangeEnabled = c.aprs_al_near;
  a.wxEnabled = c.alrt_wx_en;
  a.ledEnabled = c.led_al_en;
  a.alertKm = c.alrt_dst_km;
  a.minSeconds = (uint16_t)c.aprs_alrt_sec;
  if (cfgSeen && memcmp(&a, &lastCfg, sizeof a) == 0) return;
  lastCfg = a;
  cfgSeen = true;
  rules.configure(a);
}
}  // namespace

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

  Config c;
  cfgGet(c);
  refreshRules(c);
  if (!rules.shouldAlert(st, millis())) return;
  UiEvent e{};
  e.kind = EventKind::AprsAlert;
  e.station = st;
  postUiEvent(e);
  if (c.led_al_en && !st.isWx) rgbLedBlink(RGB_BLUE, (uint32_t)c.led_al_dur, (uint32_t)c.led_al_blk);
}
