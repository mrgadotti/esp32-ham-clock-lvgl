#pragma once
#include <stdint.h>
// Days since 1970-01-01 for a civil date (Hinnant's algorithm); newlib has no timegm.
int64_t daysFromCivil(int y, int m, int d);
// GPS ddmmyy + hhmmss (UTC) -> Unix seconds. False for unknown/implausible values (year < 2024, bad fields).
bool gpsToUnix(uint32_t ddmmyy, uint32_t hhmmss, int64_t& out);
