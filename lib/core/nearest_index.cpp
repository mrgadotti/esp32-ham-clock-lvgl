#include "nearest_index.h"

size_t nearestIndex(const int32_t* vals, size_t n, int32_t v) {
  size_t best = 0;
  for (size_t i = 1; i < n; i++) {
    int32_t d = vals[i] - v, db = vals[best] - v;
    if ((d < 0 ? -d : d) < (db < 0 ? -db : db)) best = i;
  }
  return best;
}
