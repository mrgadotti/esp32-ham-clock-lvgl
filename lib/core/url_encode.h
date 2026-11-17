#pragma once
#include <stddef.h>
// Percent-encodes everything except A-Za-z0-9 - _ . ~ ; always NUL-terminates (cap >= 1), truncates on a whole-escape boundary.
void urlEncode(const char* in, char* out, size_t cap);
