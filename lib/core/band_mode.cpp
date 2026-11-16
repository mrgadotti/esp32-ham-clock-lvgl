#include "band_mode.h"
#include <string.h>
#include <ctype.h>

// Ranges from getBand (orig L13357), extended with 60m, 6m and VHF/UHF/SHF.
Band bandFromKhz(uint32_t k) {
  struct R { uint32_t lo, hi; Band b; };
  static const R t[] = {
    {1810, 2000, B160}, {3500, 3800, B80}, {5250, 5450, B60}, {7000, 7200, B40},
    {10100, 10150, B30}, {14000, 14350, B20}, {18068, 18168, B17}, {21000, 21450, B15},
    {24890, 24990, B12}, {28000, 29700, B10}, {50000, 54000, B6},
    {30000, 299999, VHF}, {300000, 2999999, UHF}, {3000000, 300000000, SHF}};
  for (const R& r : t) if (k >= r.lo && k <= r.hi) return r.b;
  return BAND_UNKNOWN;
}

const char* bandName(Band b) {
  static const char* const n[] = {"160m","80m","60m","40m","30m","20m","17m","15m","12m","10m","6m","VHF","UHF","SHF","Other"};
  return b <= BAND_UNKNOWN ? n[b] : "Other";
}

static bool has(const char* s, const char* w) { return strstr(s, w) != nullptr; }

// Comment keywords first (orig getMode L13373), then CW sub-band by frequency, else SSB.
Mode modeFrom(const char* comment, uint32_t khz) {
  char u[48] = {0};
  for (size_t i = 0; comment && comment[i] && i < sizeof(u) - 1; ++i) u[i] = toupper((unsigned char)comment[i]);
  if (has(u, "CW")) return MODE_CW;
  if (has(u, "FT8") || has(u, "FT4") || has(u, "RTTY") || has(u, "PSK") || has(u, "JT") || has(u, "WSPR") || has(u, "DIGI")) return MODE_DIGI;
  if (has(u, "SSB") || has(u, "USB") || has(u, "LSB")) return MODE_SSB;
  struct R { uint32_t lo, hi; };  // CW segments
  static const R cw[] = {{1810,1840},{3500,3600},{5250,5354},{7000,7040},{10100,10150},
                         {14000,14070},{18068,18095},{21000,21070},{24890,24915},{28000,28070}};
  for (const R& r : cw) if (khz >= r.lo && khz <= r.hi) return MODE_CW;
  return MODE_SSB;
}

bool passesFilter(Band b, Mode m, uint16_t bandMask, uint8_t modeMask) {
  if (bandMask && !(bandMask & (1u << b))) return false;
  if (modeMask) {
    uint8_t bit = m == MODE_CW ? 1 : m == MODE_SSB ? 2 : m == MODE_DIGI ? 4 : 0;
    if (!(modeMask & bit)) return false;
  }
  return true;
}
