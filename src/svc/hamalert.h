#pragma once
#include <stdint.h>

// Net task: every 60 s opens a telnet session to HamAlert, runs `sh/dx 30` and replaces g_store.hamalert.
// Non-blocking state machine (async connect, banner, login, line reads): each tick does a few ms of work at most.
void hamalertTick(uint32_t nowMs);
