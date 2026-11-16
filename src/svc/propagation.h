#pragma once
#include <stdint.h>

void propTick(uint32_t nowMs);  // net task; fetches hamqsl solar XML hourly, retry 5 min after failure
