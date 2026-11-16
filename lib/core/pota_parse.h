#pragma once
#include <stdint.h>
#include <stddef.h>
#include <ArduinoJson.h>
#include "types.h"
// "YYYY-MM-DDTHH:MM:SS" (UTC, optional trailing fraction/Z) -> Unix seconds. False on malformed input.
bool isoUtcToUnix(const char* iso, uint32_t& out);
// JSON number or numeric string -> double (finite, strict). False otherwise.
bool jsonToDouble(JsonVariantConst v, double& out);
// Copies printable ASCII only (others become ?), always NUL-terminated.
void copyPrintable(char* dst, size_t cap, const char* src);
// api.pota.app/v1/spots entry -> Spot (khz, mode, call, utc, country=locationDesc). Out untouched on false.
bool potaFromJson(JsonObjectConst o, Spot& out);
// In-place sort, newest first (insertion sort, no heap).
void potaSortByTimeDesc(Spot* arr, size_t n);
