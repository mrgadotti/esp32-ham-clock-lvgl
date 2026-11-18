#include "aprs_sort.h"
#include <string.h>

bool aprsBefore(const AprsStation& a, const AprsStation& b, int sort) {
  if (sort == APRS_SORT_CALL) return strcmp(a.call, b.call) < 0;
  if (sort != APRS_SORT_DIST) return false;
  bool ak = a.hasLatLon && a.distKm >= 0, bk = b.hasLatLon && b.distKm >= 0;
  if (ak != bk) return ak;  // unknown distances last
  return ak && a.distKm < b.distKm;
}
