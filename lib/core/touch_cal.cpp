#include "touch_cal.h"

#include <math.h>

bool touchCalSolve(const int16_t raw[3][2], const int16_t scr[3][2], TouchCal& out) {
  double x0 = raw[0][0], y0 = raw[0][1], x1 = raw[1][0], y1 = raw[1][1], x2 = raw[2][0], y2 = raw[2][1];
  double det = x0 * (y1 - y2) - y0 * (x1 - x2) + (x1 * y2 - x2 * y1);
  if (fabs(det) < 1.0) return false;
  // Cramer's rule on [rx ry 1] * [p q r]^T = t, once per screen axis.
  auto solve = [&](double t0, double t1, double t2, float& p, float& q, float& r) {
    p = (float)((t0 * (y1 - y2) - y0 * (t1 - t2) + (t1 * y2 - t2 * y1)) / det);
    q = (float)((x0 * (t1 - t2) - t0 * (x1 - x2) + (x1 * t2 - x2 * t1)) / det);
    r = (float)((x0 * (y1 * t2 - y2 * t1) - y0 * (x1 * t2 - x2 * t1) + t0 * (x1 * y2 - x2 * y1)) / det);
  };
  solve(scr[0][0], scr[1][0], scr[2][0], out.a, out.b, out.c);
  solve(scr[0][1], scr[1][1], scr[2][1], out.d, out.e, out.f);
  return true;
}

static int16_t clampi(float v, int16_t hi) {
  int r = (int)lroundf(v);
  return r < 0 ? 0 : (r > hi ? hi : r);
}

void touchCalApply(const TouchCal& t, int16_t rx, int16_t ry, int16_t& x, int16_t& y) {
  x = clampi(t.a * rx + t.b * ry + t.c, 319);
  y = clampi(t.d * rx + t.e * ry + t.f, 239);
}

TouchCal touchCalDefault() {
  const float sx = 319.0f / 3600.0f, sy = 239.0f / 3600.0f;
  return {sx, 0, -200 * sx, 0, sy, -200 * sy};
}
