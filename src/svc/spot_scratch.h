#pragma once
#include <stddef.h>

#include "types.h"

// One static scratch for the 30-spot pollers (POTA, HamAlert; both run in the net task, one at a time), instead of
// a malloc/free per poll. Only valid inside a single tick: never keep the pointer across ticks.
constexpr size_t kSpotScratchN = 30;
inline Spot* spotScratch() {
  static Spot a[kSpotScratchN];
  return a;
}
