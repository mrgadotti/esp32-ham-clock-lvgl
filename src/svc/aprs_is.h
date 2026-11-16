#pragma once
#include <stdint.h>
void aprsIsTick(uint32_t now);  // net task: connect/login/filter/read; off without callsign or when UART2 is an APRS TNC
