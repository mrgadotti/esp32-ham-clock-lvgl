#include "hold_detector.h"

void HoldDetector::press(int x, int y, uint32_t now) { active_ = true; x_ = x; y_ = y; start_ = now; }
void HoldDetector::move(int x, int y) {
  int dx = x - x_, dy = y - y_;
  if (dx * dx + dy * dy > kSlopPx * kSlopPx) active_ = false;
}
void HoldDetector::release() { active_ = false; }
bool HoldDetector::update(uint32_t now) {
  if (!active_ || now - start_ < kHoldMs) return false;
  active_ = false;  // fires once per press
  return true;
}
