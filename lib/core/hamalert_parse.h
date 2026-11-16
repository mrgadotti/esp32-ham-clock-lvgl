#pragma once
#include <stdint.h>
#include "types.h"
// Accepts a set/json JSON line or a DX-format line. nowUtc = Unix seconds. Out untouched on false.
bool hamalertParseLine(const char* line, uint32_t nowUtc, Spot& out);
