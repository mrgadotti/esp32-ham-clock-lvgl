#include "geo.h"
#include <math.h>
#include <string.h>
#include <ctype.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Ported from latLonToMaidenhead6 (orig L4205)
bool latLonToLocator(double lat, double lon, char out[7]) {
  if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0) { strcpy(out, "--"); return false; }
  if (lat >= 90.0) lat = 89.999999;
  if (lon >= 180.0) lon = 179.999999;
  double lonAdj = lon + 180.0, latAdj = lat + 90.0;
  int fl = (int)(lonAdj / 20.0), fa = (int)(latAdj / 10.0);
  double lo = lonAdj - fl * 20.0, la = latAdj - fa * 10.0;
  int sl = (int)(lo / 2.0), sa = (int)(la / 1.0);
  if (sl > 9) sl = 9;
  if (sa > 9) sa = 9;
  int bl = (int)((lo - sl * 2.0) * 12.0), ba = (int)((la - sa) * 24.0);
  if (bl > 23) bl = 23;
  if (ba > 23) ba = 23;
  out[0] = 'A' + fl; out[1] = 'A' + fa; out[2] = '0' + sl; out[3] = '0' + sa;
  out[4] = 'a' + bl; out[5] = 'a' + ba; out[6] = 0;
  return true;
}

// Ported from locatorToLatLon (orig L12786); adds validation and subsquare centre for 6 chars.
bool locatorToLatLon(const char* loc, double& lat, double& lon) {
  size_t n = loc ? strlen(loc) : 0;
  if (n != 4 && n != 6) return false;
  int c0 = toupper((unsigned char)loc[0]) - 'A', c1 = toupper((unsigned char)loc[1]) - 'A';
  int d0 = loc[2] - '0', d1 = loc[3] - '0';
  if (c0 < 0 || c0 > 17 || c1 < 0 || c1 > 17 || d0 < 0 || d0 > 9 || d1 < 0 || d1 > 9) return false;
  double lo = c0 * 20 - 180 + d0 * 2, la = c1 * 10 - 90 + d1;
  if (n == 6) {
    int s0 = toupper((unsigned char)loc[4]) - 'A', s1 = toupper((unsigned char)loc[5]) - 'A';
    if (s0 < 0 || s0 > 23 || s1 < 0 || s1 > 23) return false;
    lon = lo + s0 / 12.0 + 1.0 / 24.0;
    lat = la + s1 / 24.0 + 1.0 / 48.0;
  } else {
    lon = lo + 1.0;
    lat = la + 0.5;
  }
  return true;
}

// Ported from calculateDistance (orig L13299); radius 6366.71 km as in the original
float distanceKm(double lat1, double lon1, double lat2, double lon2) {
  const double r = 6366.71;
  double dLat = (lat2 - lat1) * M_PI / 180.0, dLon = (lon2 - lon1) * M_PI / 180.0;
  double a = sin(dLat / 2) * sin(dLat / 2) +
             cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) * sin(dLon / 2) * sin(dLon / 2);
  return (float)(r * 2.0 * atan2(sqrt(a), sqrt(1.0 - a)));
}

// Ported from calculateBearing (orig L13313)
float bearingDeg(double lat1, double lon1, double lat2, double lon2) {
  double p1 = lat1 * M_PI / 180.0, p2 = lat2 * M_PI / 180.0, dLon = (lon2 - lon1) * M_PI / 180.0;
  double y = sin(dLon) * cos(p2);
  double x = cos(p1) * sin(p2) - sin(p1) * cos(p2) * cos(dLon);
  double b = atan2(y, x) * 180.0 / M_PI;
  if (b < 0.0) b += 360.0;
  return (float)b;
}
