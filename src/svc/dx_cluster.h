#pragma once
#include <stdint.h>

// Net task: telnet DX cluster client. Non-blocking reads, parsed spots go to g_store.dx.
void dxTick(uint32_t nowMs);
