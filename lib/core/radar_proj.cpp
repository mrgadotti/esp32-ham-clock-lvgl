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
RadarView radarZoomAt(const RadarView& v, int16_t tapX, int16_t tapY, float rangeKm, int16_t radiusPx,
                      int16_t originX, int16_t originY) {
  const float s = radiusPx / rangeKm * v.zoom;
  if (!(s > 0) || !isfinite(s)) return v;
  RadarView n;
  n.cxKm = v.cxKm + (tapX - originX) / s;
  n.cyKm = v.cyKm - (tapY - originY) / s;
  n.zoom = v.zoom * 2 > 10 ? 10 : v.zoom * 2;
  return n;
}
