#pragma once
#include <stdint.h>
#include "types.h"
bool nmeaChecksumOk(const char* sentence);  // no '*' => accepted (as the original)
// Updates `gps` from RMC/GGA/GSV or a $GPSESP32[...] frame. Returns true if the sentence was recognised and valid.
// nowMs is injected (deviation from the brief signature) so lastFixMs stays pure.
bool nmeaParse(const char* sentence, GpsState& gps, uint32_t nowMs);
