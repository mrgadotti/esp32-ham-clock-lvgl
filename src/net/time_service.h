#pragma once
#include <stdint.h>
#include <time.h>

bool timeValid();  // year >= 2024
void timeTick(uint32_t now);  // NTP start + hourly resync, skipped while GPS time is valid
bool ntpSyncedWithin(uint32_t ms);  // an SNTP sync happened in the last ms
void localTimeOf(time_t utc, struct tm& out);  // applies cfg timezone (float hours)
