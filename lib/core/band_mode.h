#pragma once
#include <stdint.h>
enum Band : uint8_t { BAND_160M, BAND_80M, BAND_60M, BAND_40M, BAND_30M, BAND_20M, BAND_17M, BAND_15M, BAND_12M, BAND_10M, BAND_6M, BAND_VHF, BAND_UHF, BAND_SHF, BAND_UNKNOWN };
enum Mode : uint8_t { MODE_CW, MODE_SSB, MODE_DIGI, MODE_UNKNOWN };
Band bandFromKhz(uint32_t khz);
const char* bandName(Band b);
Mode modeFrom(const char* comment, uint32_t khz);
// bandMask: bit = Band value; modeMask: CW=1, SSB=2, DIGI=4; 0 = ALL
bool passesFilter(Band b, Mode m, uint16_t bandMask, uint8_t modeMask);
