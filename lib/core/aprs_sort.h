#pragma once
#include "types.h"

enum AprsSortMode { APRS_SORT_TIME, APRS_SORT_CALL, APRS_SORT_DIST };

// Strict ordering for a stable insertion sort: CALL ascending; DIST ascending with unknown distances last.
// TIME has no ordering (the ring is already newest first), so it never reorders.
bool aprsBefore(const AprsStation& a, const AprsStation& b, int sort);
