#pragma once
#include <stddef.h>
#include <stdint.h>

// Copies at most maxBytes of s to out (capacity cap) without leaving half of a UTF-8 sequence at the cut.
void textCut(char* out, size_t cap, const char* s, size_t maxBytes);
// Distance for a 4-character table cell: "999", "9999", "12k" (>= 10000 km); negative values are clamped to 0.
// UTC time-of-day cell "HH:MM" from epoch seconds; "--" for 0 (unknown).
void formatHmCell(char* out, size_t cap, uint32_t utc);
void formatKmCell(char* out, size_t cap, int km);
