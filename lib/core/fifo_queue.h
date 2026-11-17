#pragma once
#include <stddef.h>

// Fixed-storage FIFO (no heap). When full, push() drops the OLDEST entry so the newest alert is kept.
template <typename T, size_t N>
class FifoQueue {
 public:
  // True if stored without loss; false if the oldest entry was dropped to make room.
  bool push(const T& v) {
    bool lossless = n_ < N;
    if (!lossless) { head_ = (head_ + 1) % N; --n_; }
    a_[(head_ + n_) % N] = v;
    ++n_;
    return lossless;
  }
  bool pop(T& out) {
    if (!n_) return false;
    out = a_[head_];
    head_ = (head_ + 1) % N;
    --n_;
    return true;
  }
  size_t size() const { return n_; }
  bool empty() const { return n_ == 0; }
 private:
  T a_[N];
  size_t head_ = 0;
  size_t n_ = 0;
};
