#include "gps_time.h"

int64_t daysFromCivil(int y, int m, int d) {
  y -= m <= 2;
  int era = (y >= 0 ? y : y - 399) / 400;
  int yoe = y - era * 400;
  int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return (int64_t)era * 146097 + doe - 719468;
}

bool gpsToUnix(uint32_t date, uint32_t utc, int64_t& out) {
  if (!date || !utc) return false;
  int d = date / 10000, mo = date / 100 % 100, y = 2000 + date % 100;
  int hh = utc / 10000, mi = utc / 100 % 100, ss = utc % 100;
  if (y < 2024 || mo < 1 || mo > 12 || d < 1 || d > 31 || hh > 23 || mi > 59 || ss > 59) return false;
  out = daysFromCivil(y, mo, d) * 86400 + hh * 3600 + mi * 60 + ss;
  return true;
}
