#pragma once
#include <stdint.h>

// Long-press detector: fires once when a press stays within kSlopPx of its start point for kHoldMs.
class HoldDetector {
 public:
  static constexpr uint32_t kHoldMs = 15000;
  static constexpr int kSlopPx = 20;
  void press(int x, int y, uint32_t now);
  void move(int x, int y);  // beyond the slop cancels the hold
  void release();
  bool update(uint32_t now);  // true once per press, when kHoldMs has elapsed
  bool active() const { return active_; }
  uint32_t elapsed(uint32_t now) const { return active_ ? now - start_ : 0; }

 private:
  bool active_ = false;
  int x_ = 0, y_ = 0;
  uint32_t start_ = 0;
};
