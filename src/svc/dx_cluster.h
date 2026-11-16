#pragma once
#include <stdint.h>
#include "types.h"

// Net task: telnet DX cluster client. Non-blocking reads, parsed spots go to g_store.dx.
void dxTick(uint32_t nowMs);
// Called for every parsed spot before it is stored (weak no-op; QRZ enrichment overrides it in Task 11).
void qrzOnNewSpot(const Spot& s);
