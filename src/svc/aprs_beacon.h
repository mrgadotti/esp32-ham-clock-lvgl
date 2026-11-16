#pragma once
#include <stdint.h>
void aprsBeaconTick(uint32_t nowMs);  // net task: periodic position beacon over APRS-IS or UART2 KISS
