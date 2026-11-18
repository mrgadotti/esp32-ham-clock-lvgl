#pragma once
#include <stdint.h>

void weatherOmTick(uint32_t nowMs);  // net task; idle unless scr_en bit 10 or 11 is set
