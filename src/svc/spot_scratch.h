#pragma once
#include <stddef.h>

#include "types.h"

// One static scratch for the 30-spot pollers (POTA, HamAlert; both run in the net task, one at a time), instead of
// a malloc/free per poll. POTA fills and publishes it inside one tick; HamAlert keeps it across ticks and raises
// spotScratchHeld() meanwhile, so POTA waits for it.
constexpr size_t kSpotScratchN = 30;
inline Spot* spotScratch() {
  static Spot a[kSpotScratchN];
  return a;
}
inline bool& spotScratchHeld() {
  static bool held;
  return held;
}
