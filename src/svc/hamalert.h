#pragma once
#include <stdint.h>

// Net task: every 60 s opens a telnet session to HamAlert, runs `sh/dx 30` and replaces g_store.hamalert.
// Blocks the net task for up to ~12 s per poll (same as the original).
void hamalertTick(uint32_t nowMs);
