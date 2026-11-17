#include "radar_proj.h"
#include <math.h>
RadarView radarReset() { return {1.0f, 0.0f, 0.0f}; }
bool radarProject(const RadarView& v, float distKm, float bearingDeg, float rangeKm, int16_t radiusPx,
                  int16_t originX, int16_t originY, int16_t& px, int16_t& py) {
  const float rad = bearingDeg * 3.14159265f / 180.0f, s = radiusPx / rangeKm * v.zoom;
  if (!(s > 0) || !isfinite(s) || !isfinite(distKm) || !isfinite(bearingDeg)) return false;
  const float x = originX + (distKm * sinf(rad) - v.cxKm) * s;
  const float y = originY - (distKm * cosf(rad) - v.cyKm) * s;
  const int ix = (int)lroundf(x), iy = (int)lroundf(y);
  if (ix < 0 || ix >= 320 || iy < 0 || iy >= 240) return false;
  px = (int16_t)ix; py = (int16_t)iy;
  return true;
}
RadarView radarZoomStep(const RadarView& v, int dir) {
  static const float kZ[] = {1, 2, 4, 8, 10, 15, 20};
  const int n = sizeof kZ / sizeof kZ[0];
  int i = 0;  // nearest preset to the current zoom
  for (int k = 1; k < n; k++) if (fabsf(kZ[k] - v.zoom) < fabsf(kZ[i] - v.zoom)) i = k;
  i += dir > 0 ? 1 : dir < 0 ? -1 : 0;
  RadarView r = v;
  r.zoom = kZ[i < 0 ? 0 : i >= n ? n - 1 : i];
  return r;
}
