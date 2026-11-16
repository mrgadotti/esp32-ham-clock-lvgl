#include "backoff.h"
uint32_t backoffMs(uint8_t failures) {
  if (failures >= 6) return 300000;  // 5000 << 6 = 320000 > cap
  uint32_t v = 5000u << failures;
  return v > 300000 ? 300000 : v;
}
