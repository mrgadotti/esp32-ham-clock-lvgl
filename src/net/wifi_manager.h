#pragma once
#include <stdint.h>

void wifiTick(uint32_t now);  // net task; STA (ssid1, ssid2) then AP fallback; writes g_store.net
