#include "service_coords.h"

#include "config_store.h"
#include "geo.h"

bool serviceCoords(double& lat, double& lon) {
  // GPS branch: enabled in Task 17.
  Config c;
  cfgGet(c);
  if (c.user_ll_ok) {
    lat = c.user_lat;
    lon = c.user_lon;
    return true;
  }
  return locatorToLatLon(c.user_locator, lat, lon);
}
