#include "dx_parse.h"
#include "band_mode.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static void copyTok(char* dst, size_t cap, const char* s, size_t n) {
  if (n >= cap) n = cap - 1;
  memcpy(dst, s, n); dst[n] = 0;
}

static char up(char c) { return (char)toupper((unsigned char)c); }

// Maidenhead 4 or 6 chars: field A-R, subsquare A-X, case-insensitive.
static bool isLoc(const char* p, size_t len) {
  if (up(p[0]) < 'A' || up(p[0]) > 'R' || up(p[1]) < 'A' || up(p[1]) > 'R') return false;
  if (!isdigit((unsigned char)p[2]) || !isdigit((unsigned char)p[3])) return false;
  if (len == 6) return up(p[4]) >= 'A' && up(p[4]) <= 'X' && up(p[5]) >= 'A' && up(p[5]) <= 'X';
  return true;
}

bool parseDxLine(const char* line, uint32_t nowUtc, Spot& out) {
  if (!line || strncmp(line, "DX de", 5) != 0) return false;
  const char* colon = strchr(line + 5, ':');
  if (!colon) return false;
  memset(&out, 0, sizeof(out));
  const char* p = line + 5;
  while (p < colon && *p == ' ') p++;
  const char* e = colon;
  while (e > p && e[-1] == ' ') e--;
  copyTok(out.spotter, sizeof(out.spotter), p, e - p);

  p = colon + 1;
  while (*p == ' ') p++;
  char* end;
  double f = strtod(p, &end);
  if (end == p || f <= 0 || (*end != ' ' && *end != 0)) return false;
  out.khz = (uint32_t)(f + 0.5);
  p = end;
  while (*p == ' ') p++;
  e = p;
  while (*e && *e != ' ') e++;
  if (e == p) return false;
  copyTok(out.call, sizeof(out.call), p, e - p);
  p = e;
  while (*p == ' ') p++;

  // Trailing "HHMMZ" token is the spot time; the rest is the comment.
  size_t n = strlen(p);
  while (n && p[n - 1] == ' ') n--;
  uint32_t hhmm = 0; bool hasTime = false;
  if (n >= 5 && p[n - 1] == 'Z' && (n == 5 || p[n - 6] == ' ')) {
    bool ok = true;
    for (int i = 0; i < 4; i++) ok = ok && isdigit((unsigned char)p[n - 5 + i]);
    if (ok) {
      hhmm = (p[n-5]-'0')*1000 + (p[n-4]-'0')*100 + (p[n-3]-'0')*10 + (p[n-2]-'0');
      if (hhmm / 100 < 24 && hhmm % 100 < 60) { hasTime = true; n -= 5; }
    }
  }
  while (n && p[n - 1] == ' ') n--;

  // Sanitize: printable ASCII only, truncated to fit.
  size_t o = 0;
  for (size_t i = 0; i < n && o < sizeof(out.comment) - 1; i++)
    out.comment[o++] = (p[i] >= 0x20 && p[i] < 0x7F) ? p[i] : '?';
  out.comment[o] = 0;

  for (size_t i = 0; i + 4 <= o; i++) {
    size_t len = (i + 6 <= o && isLoc(out.comment + i, 6)) ? 6 : (isLoc(out.comment + i, 4) ? 4 : 0);
    if (!len) continue;
    bool lb = i == 0 || !isalnum((unsigned char)out.comment[i - 1]);
    bool rb = i + len == o || !isalnum((unsigned char)out.comment[i + len]);
    if (lb && rb) {
      for (size_t k = 0; k < len; k++) out.loc[k] = up(out.comment[i + k]);
      break;
    }
  }

  uint32_t day = nowUtc - nowUtc % 86400;
  uint32_t t = hasTime ? (hhmm / 100) * 3600 + (hhmm % 100) * 60 : nowUtc % 86400;
  // A spot time more than 1 h ahead of now belongs to the previous day.
  if (hasTime && t > nowUtc % 86400 + 3600 && day >= 86400) day -= 86400;
  out.utc = day + t;
  out.band = bandFromKhz(out.khz);
  out.mode = modeFrom(out.comment, out.khz);
  out.distKm = -1;
  out.hasLatLon = false;
  return true;
}
