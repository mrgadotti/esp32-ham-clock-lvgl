#pragma once
#include <stdint.h>

void weatherTick(uint32_t nowMs);  // net task; at most one HTTP request per call
