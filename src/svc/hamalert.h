#pragma once
#include <stdint.h>

// Net task: every 60 s opens a telnet session to HamAlert, runs `sh/dx 30` and replaces g_store.hamalert.
// Blocks the net task for up to ~15 s per poll (4 s connect + 1.2 s banner + 0.6 s login delays + 9 s read).
void hamalertTick(uint32_t nowMs);
