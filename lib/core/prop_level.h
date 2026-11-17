#pragma once
// Propagation colour categories; src/ui maps them to lv_color_t.

enum class PropCond : unsigned char { Unknown, Good, Fair, Poor };
// Case-insensitive substring match on the first 7 chars (as in the original): GOOD / FAIR / POOR, else Unknown.
PropCond propCondOf(const char* s);

enum class KLevel : unsigned char { None, Low, Mid, High };
// Planetary K-index text: <4 Low, <6 Mid, else High; None if not a number (neutral colour).
KLevel kIndexLevel(const char* s);
