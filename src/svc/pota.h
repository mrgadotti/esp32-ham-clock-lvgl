#pragma once
#include <stdint.h>

// Net task: POTA spots from the HTTP API every 180 s into g_store.pota (newest first, max 30).
void potaTick(uint32_t nowMs);
