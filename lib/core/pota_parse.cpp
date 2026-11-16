#include "band_mode.h"  // before ArduinoJson: Arduino.h defines B10 etc.
#include "pota_parse.h"
#include "band_mode.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static bool dig(const char* p, int n, int& v) {
  v = 0;
  for (int i = 0; i < n; i++) {
    if (p[i] < '0' || p[i] > '9') return false;
    v = v * 10 + (p[i] - '0');
  }
  return true;
}

bool isoUtcToUnix(const char* s, uint32_t& out) {
  if (!s || strlen(s) < 19) return false;
  int y, mo, d, h, mi, se;
  if (!dig(s, 4, y) || s[4] != '-' || !dig(s + 5, 2, mo) || s[7] != '-' || !dig(s + 8, 2, d) ||
      (s[10] != 'T' && s[10] != ' ') || !dig(s + 11, 2, h) || s[13] != ':' || !dig(s + 14, 2, mi) ||
      s[16] != ':' || !dig(s + 17, 2, se))
    return false;
  static const uint8_t dim[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  bool leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
  if (y < 1970 || y > 2105 || mo < 1 || mo > 12 || d < 1 || d > dim[mo - 1] + (mo == 2 && leap) || h > 23 || mi > 59 || se > 60)
    return false;
  // Days from civil (Howard Hinnant), valid for y >= 1970.
  int yy = y - (mo <= 2);
  int era = yy / 400, yoe = yy - era * 400;
  int doy = (153 * (mo + (mo > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  int64_t days = (int64_t)era * 146097 + doe - 719468;
  int64_t t = days * 86400 + h * 3600 + mi * 60 + se;
  if (t < 0 || t > 0xFFFFFFFFLL) return false;
  out = (uint32_t)t;
  return true;
}

bool jsonToDouble(JsonVariantConst v, double& out) {
  if (v.is<const char*>()) {
    const char* s = v.as<const char*>();
    char* end;
    double d = strtod(s, &end);
    if (end == s || *end != 0 || !isfinite(d)) return false;
    out = d;
    return true;
  }
  if (v.is<double>()) { out = v.as<double>(); return isfinite(out); }
  return false;
}

void copyPrintable(char* dst, size_t cap, const char* src) {
  size_t o = 0;
  for (size_t i = 0; src && src[i] && o < cap - 1; i++)
    dst[o++] = (src[i] >= 0x20 && src[i] < 0x7F) ? src[i] : '?';
  dst[o] = 0;
}

bool potaFromJson(JsonObjectConst o, Spot& dst) {
  const char* call = o["activator"] | "";
  double f;
  if (!*call || !jsonToDouble(o["frequency"], f) || f <= 0 || f >= 1e7) return false;
  Spot s{};
  copyPrintable(s.call, sizeof(s.call), call);
  copyPrintable(s.spotter, sizeof(s.spotter), o["spotter"] | "");
  s.khz = (uint32_t)(f + 0.5);
  s.band = bandFromKhz(s.khz);
  const char* loc = o["locationDesc"] | "";
  copyPrintable(s.country, sizeof(s.country), loc);
  char buf[96];
  snprintf(buf, sizeof(buf), "%s %s", (const char*)(o["reference"] | ""), loc);
  copyPrintable(s.comment, sizeof(s.comment), buf);
  const char* mode = o["mode"] | "";
  s.mode = modeFrom(*mode ? mode : (const char*)(o["comments"] | ""), s.khz);
  if (!isoUtcToUnix(o["spotTime"] | "", s.utc)) s.utc = 0;
  s.distKm = -1;
  dst = s;
  return true;
}

void potaSortByTimeDesc(Spot* a, size_t n) {
  for (size_t i = 1; i < n; i++) {
    Spot t = a[i];
    size_t j = i;
    while (j > 0 && a[j - 1].utc < t.utc) { a[j] = a[j - 1]; j--; }
    a[j] = t;
  }
}
