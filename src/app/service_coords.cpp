#include "service_coords.h"

#include <Arduino.h>
#include "config_store.h"
#include "data_store.h"
#include "geo.h"

namespace {
// Config-derived inputs, refreshed only when cfgVersion() moves (no 1.3 KB Config copy per call).
struct Fixed {
  bool gpsAuto, ok;  // ok: lat/lon valid (user lat/lon, else locator)
  double lat, lon;
};
Fixed s_fixed;
uint32_t s_ver = ~0u;
portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;  // spans a 24-byte copy: callers come from the net and UI tasks
}  // namespace

bool serviceCoords(double& lat, double& lon) {
  uint32_t ver = cfgVersion();  // read before the config: a racing update only costs one extra refresh
  Fixed f;
  portENTER_CRITICAL(&s_mux);
  bool hit = s_ver == ver;
  if (hit) f = s_fixed;
  portEXIT_CRITICAL(&s_mux);
  if (!hit) {
    cfgRead([&](const Config& c) {
      f.gpsAuto = c.gps_auto_pos;
      f.ok = c.user_ll_ok;
      f.lat = c.user_lat;
      f.lon = c.user_lon;
      if (!f.ok) f.ok = locatorToLatLon(c.user_locator, f.lat, f.lon);
    });
    portENTER_CRITICAL(&s_mux);
    s_fixed = f;
    s_ver = ver;
    portEXIT_CRITICAL(&s_mux);
  }
  if (f.gpsAuto) {
    GpsState g;
    g_store.gps.read([&](const GpsState& v) { g = v; });
    if (g.fix && millis() - g.lastFixMs < 15000) {
      lat = g.lat;
      lon = g.lon;
      return true;
    }
  }
  if (f.ok) {
    lat = f.lat;
    lon = f.lon;
  }
  return f.ok;
}
