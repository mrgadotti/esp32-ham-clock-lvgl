#pragma once
#include <stdint.h>
#include <time.h>

bool timeValid();  // year >= 2024
void timeTick(uint32_t now);  // NTP start + hourly resync
