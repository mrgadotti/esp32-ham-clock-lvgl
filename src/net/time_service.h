#pragma once
#include <stdint.h>
#include <time.h>

bool timeValid();  // year >= 2024
void timeTick(uint32_t now);  // NTP start + hourly resync, skipped while GPS time is valid
void localTimeOf(time_t utc, struct tm& out);  // applies cfg timezone (float hours)
