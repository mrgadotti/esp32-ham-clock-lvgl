#pragma once
#include <stddef.h>
#include <stdint.h>

// Index of the entry of vals[0..n) closest to v (the first one on a tie); 0 when n is 0.
size_t nearestIndex(const int32_t* vals, size_t n, int32_t v);
