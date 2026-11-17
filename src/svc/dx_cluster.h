#pragma once
#include <stdint.h>
#include "types.h"

// Net task: telnet DX cluster client. Non-blocking reads, parsed spots go to g_store.dx.
void dxTick(uint32_t nowMs);
// Called for every parsed spot before it is stored (defined in qrz.cpp).
void qrzOnNewSpot(const Spot& s);
