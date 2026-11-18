#pragma once
#include <stddef.h>

// Radar range label: "150km" (>= 100), "12.5km" (>= 10), "1.25km" (below).
void formatRadarKm(float km, char* out, size_t n);
