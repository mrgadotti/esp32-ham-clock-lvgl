#include "nmea.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

bool nmeaChecksumOk(const char* s) {
  if (!s || s[0] != '$') return false;
  const char* star = strchr(s, '*');
  if (!star) return true;
  if (!isxdigit((unsigned char)star[1]) || !isxdigit((unsigned char)star[2])) return false;
  uint8_t calc = 0;
  for (const char* p = s + 1; p < star; p++) calc ^= (uint8_t)*p;
  char hex[3] = {star[1], star[2], 0};
  return calc == (uint8_t)strtoul(hex, nullptr, 16);
}

// Copies field `idx` (comma/star separated) into out. Returns its length.
static size_t field(const char* s, int idx, char* out, size_t cap) {
  int cur = 0;
  const char* start = s;
  for (const char* p = s;; p++) {
    bool end = (*p == 0 || *p == ',' || *p == '*');
    if (!end) continue;
    if (cur == idx) {
      size_t n = (size_t)(p - start);
      if (n >= cap) n = cap - 1;
      memcpy(out, start, n);
      out[n] = 0;
      return n;
    }
    if (*p == 0 || *p == '*') break;
    cur++;
    start = p + 1;
  }
  out[0] = 0;
  return 0;
}

static bool coord(const char* raw, char hem, double maxDeg, double& out) {
  size_t len = strlen(raw);
  if (len < 4) return false;
  const char* dot = strchr(raw, '.');
  int dd = dot ? ((dot - raw) > 4 ? 3 : 2) : (len > 5 ? 3 : 2);
  if ((int)len <= dd) return false;
  char d[4] = {0};
  memcpy(d, raw, dd);
  double deg = atof(d), min = atof(raw + dd);
  if (min < 0.0 || min >= 60.0 || deg > maxDeg || (deg == maxDeg && min > 0.0)) return false;
  out = deg + min / 60.0;
  hem = (char)toupper((unsigned char)hem);
  if (hem == 'S' || hem == 'W') out = -out;
  return true;
}

static bool getPos(const char* s, int first, double& la, double& lo) {
  char f[4][20];
  for (int i = 0; i < 4; i++) field(s, first + i, f[i], sizeof f[i]);
  if (!f[0][0] || !f[1][0] || !f[2][0] || !f[3][0]) return false;
  return coord(f[0], f[1][0], 90.0, la) && coord(f[2], f[3][0], 180.0, lo);
}

// Fix is true only when the position parsed too; lat/lon are untouched otherwise.
static void applyFix(GpsState& g, bool valid, const char* s, int first, uint32_t now) {
  double la = 0, lo = 0;
  bool ok = valid && getPos(s, first, la, lo);
  if (ok) { g.lat = la; g.lon = lo; }
  g.fix = ok;
  if (ok) g.lastFixMs = now;
}

static uint32_t digits(const char* s, size_t n) {  // n leading chars as a number; 0 if too short
  if (strlen(s) < n) return 0;
  char b[12] = {0};
  memcpy(b, s, n);
  return (uint32_t)strtoul(b, nullptr, 10);
}

static bool parseEsp32Frame(const char* line, GpsState& g, uint32_t now) {
  const char* open = strchr(line, '[');
  const char* close = strrchr(line, ']');
  if (!open || close <= open) return false;
  char pay[96];
  size_t n = (size_t)(close - open - 1);
  if (n >= sizeof pay) return false;
  memcpy(pay, open + 1, n); pay[n] = 0;
  char f[7][20];
  int cnt = 0;
  for (char* p = pay;; cnt++) {
    if (cnt >= 7) return false;
    char* c = strchr(p, ',');
    size_t l = c ? (size_t)(c - p) : strlen(p);
    if (l >= sizeof f[0]) l = sizeof f[0] - 1;
    memcpy(f[cnt], p, l); f[cnt][l] = 0;
    if (!c) { cnt++; break; }
    p = c + 1;
  }
  if (cnt != 7) return false;
  for (int i = 0; i < 7; i++) {
    char* t = f[i];
    while (*t == ' ') t++;
    size_t l = strlen(t);
    while (l && t[l - 1] == ' ') t[--l] = 0;
    if (t != f[i]) memmove(f[i], t, l + 1);
  }
  bool fix = atoi(f[0]) == 1 && atoi(f[1]) == 1;
  int sats = atoi(f[2]);
  double la = 0, lo = 0;
  if (fix) {  // validate before touching state
    la = atof(f[3]); lo = atof(f[4]);
    if (!(la >= -90 && la <= 90 && lo >= -180 && lo <= 180)) return false;
  }
  g.active = true;
  g.sats = (uint8_t)(sats < 0 ? 0 : sats > 99 ? 99 : sats);
  g.satsGps = g.satsGlo = g.satsGal = g.satsBds = 0;
  g.speedKmh = -1.0f;
  if (strlen(f[5]) == 8 && f[5][2] == ':' && f[5][5] == ':')
    g.utc = digits(f[5], 2) * 10000 + digits(f[5] + 3, 2) * 100 + digits(f[5] + 6, 2);
  if (strlen(f[6]) == 10 && f[6][2] == '-' && f[6][5] == '-')
    g.date = digits(f[6], 2) * 10000 + digits(f[6] + 3, 2) * 100 + digits(f[6] + 8, 2);
  g.fix = fix;
  if (fix) { g.lat = la; g.lon = lo; g.lastFixMs = now; }
  else g.lat = g.lon = 0;
  return true;
}

bool nmeaParse(const char* s, GpsState& g, uint32_t now) {
  if (!s) return false;
  if (!strncmp(s, "$GPSESP32[", 10)) return parseEsp32Frame(s, g, now);
  if (!nmeaChecksumOk(s)) return false;
  char type[12], f[20];
  field(s, 0, type, sizeof type);
  size_t tl = strlen(type);
  if (tl < 5) return false;
  const char* t = type + tl - 3;  // sentence id, e.g. RMC
  if (!strcmp(t, "RMC")) {
    g.active = true;
    bool a = field(s, 2, f, sizeof f) && toupper((unsigned char)f[0]) == 'A';
    if (field(s, 1, f, sizeof f) >= 6) g.utc = digits(f, 6);
    if (field(s, 9, f, sizeof f) >= 6) g.date = digits(f, 6);
    g.speedKmh = field(s, 7, f, sizeof f) ? (float)atof(f) * 1.852f : -1.0f;
    applyFix(g, a, s, 3, now);
    return true;
  }
  if (!strcmp(t, "GGA")) {
    g.active = true;
    field(s, 6, f, sizeof f);
    bool q = atoi(f) > 0;
    if (field(s, 7, f, sizeof f)) g.sats = (uint8_t)atoi(f);
    if (field(s, 1, f, sizeof f) >= 6) g.utc = digits(f, 6);
    applyFix(g, q, s, 2, now);
    return true;
  }
  if (!strcmp(t, "GSV")) {
    if (!field(s, 3, f, sizeof f)) return false;
    int n = atoi(f);
    if (n < 0) return false;
    g.active = true;
    // GN (combined) carries no per-constellation info; ignored.
    if (!strncmp(type + 1, "GP", 2)) g.satsGps = (uint8_t)n;
    else if (!strncmp(type + 1, "GL", 2)) g.satsGlo = (uint8_t)n;
    else if (!strncmp(type + 1, "GA", 2)) g.satsGal = (uint8_t)n;
    else if (!strncmp(type + 1, "GB", 2) || !strncmp(type + 1, "BD", 2)) g.satsBds = (uint8_t)n;
    return true;
  }
  return false;
}
