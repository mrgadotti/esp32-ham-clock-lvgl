#pragma once
#include <stdint.h>
#include "types.h"
// Parses "DX de CALL: 14025.0 DXCALL comment 1234Z". nowUtc = Unix seconds (used for the date of HHMM).
bool parseDxLine(const char* line, uint32_t nowUtc, Spot& out);
