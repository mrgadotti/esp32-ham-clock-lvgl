#pragma once
#include "../../app/events.h"

void overlayShow(const UiEvent&);  // APRS/weather: cfg().alrt_scr_s seconds, beacon: 2 s
bool overlayActive();
void overlayTick(uint32_t nowMs);  // closes on timeout or X
