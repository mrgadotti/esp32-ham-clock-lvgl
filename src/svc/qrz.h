#pragma once
#include <stdint.h>

#include "types.h"

void qrzInit();  // before any task starts
void qrzTick(uint32_t nowMs);      // net task; one lookup per call at most
void qrzEnqueue(const char* call);
void qrzOnNewSpot(const Spot& s);  // enqueues when the spot has no country and is not cached
const char* qrzStatusStr();        // "disabled", "ok" or "error: ..."
