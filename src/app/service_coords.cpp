#include "service_coords.h"

#include <Arduino.h>
#include "config_store.h"
#include "data_store.h"
#include "geo.h"

bool serviceCoords(double& lat, double& lon) {
  Config c;
  cfgGet(c);
  if (c.gps_auto_pos) {
    GpsState g;
    g_store.gps.read([&](const GpsState& v) { g = v; });
    if (g.fix && millis() - g.lastFixMs < 15000) {
      lat = g.lat;
      lon = g.lon;
      return true;
    }
  }
  if (c.user_ll_ok) {
    lat = c.user_lat;
    lon = c.user_lon;
    return true;
  }
  return locatorToLatLon(c.user_locator, lat, lon);
}
