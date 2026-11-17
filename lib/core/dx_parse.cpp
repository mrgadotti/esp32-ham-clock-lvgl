#include "dx_parse.h"
#include "band_mode.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <strings.h>

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

bool parseDxLine(const char* line, uint32_t nowUtc, Spot& dst) {
  if (!line) return false;
  char buf[512];  // trimmed copy: trailing CR/LF/blanks removed
  size_t ln = strnlen(line, sizeof(buf) - 1);
  while (ln && (unsigned char)line[ln - 1] <= ' ') ln--;
  memcpy(buf, line, ln); buf[ln] = 0;
  line = buf;
  if (strncmp(line, "DX de", 5) != 0) return false;
  const char* colon = strchr(line + 5, ':');
  if (!colon) return false;
  Spot out{};  // filled locally; committed to dst only on success
  const char* p = colon + 1;
  while (*p == ' ') p++;
  char* end;
  double f = strtod(p, &end);
  if (strspn(p, "0123456789.") != (size_t)(end - p) || !isdigit((unsigned char)*p) || !isfinite(f) || f <= 0 || f >= 1e7 || (*end != ' ' && *end != 0)) return false;
  out.khz = (uint32_t)(f + 0.5);
  p = end;
  while (*p == ' ') p++;
  const char* e = p;
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

  // Locator scan on the raw (untruncated) comment.
  for (size_t i = 0; i + 4 <= n; i++) {
    size_t len = (i + 6 <= n && isLoc(p + i, 6)) ? 6 : (isLoc(p + i, 4) ? 4 : 0);
    if (!len) continue;
    if (len == 4 && strncasecmp(p + i, "RR73", 4) == 0) continue;  // FT8 sign-off, not a square
    bool lb = i == 0 || !isalnum((unsigned char)p[i - 1]);
    bool rb = i + len == n || !isalnum((unsigned char)p[i + len]);
    if (lb && rb) {
      for (size_t k = 0; k < len; k++) out.loc[k] = up(p[i + k]);
      break;
    }
  }

  // Sanitize: printable ASCII only, truncated to fit.
  size_t o = 0;
  for (size_t i = 0; i < n && o < sizeof(out.comment) - 1; i++)
    out.comment[o++] = (p[i] >= 0x20 && p[i] < 0x7F) ? p[i] : '?';
  out.comment[o] = 0;

  uint32_t day = nowUtc - nowUtc % 86400;
  uint32_t t = hasTime ? (hhmm / 100) * 3600 + (hhmm % 100) * 60 : nowUtc % 86400;
  // A spot time more than 1 h ahead of now belongs to the previous day.
  if (hasTime && t > nowUtc % 86400 + 3600 && day >= 86400) day -= 86400;
  out.utc = day + t;
  out.band = bandFromKhz(out.khz);
  out.mode = modeFrom(out.comment, out.khz);
  out.distKm = -1;
  out.hasLatLon = false;
  dst = out;
  return true;
}
