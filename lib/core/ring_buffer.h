#pragma once
#include <stddef.h>

// Newest-first, fixed storage (no heap). at(0) is the newest element. Circular: pushFront is O(1).
template <typename T, size_t N>
class RingBuffer {
 public:
  void pushFront(const T& v) {
    head_ = (head_ + N - 1) % N;  // when full this slot held the oldest element
    a_[head_] = v;
    if (n_ < N) ++n_;
  }
  T& at(size_t i) { return a_[(head_ + i) % N]; }
  const T& at(size_t i) const { return a_[(head_ + i) % N]; }
  size_t size() const { return n_; }
  void clear() { n_ = 0; head_ = 0; }
  // Replace the first match with `replacement` and move it to the front. False if none matched.
  template <class Pred>
  bool moveToFrontIf(Pred p, const T& replacement) {
    for (size_t i = 0; i < n_; ++i) {
      if (p(at(i))) {
        for (size_t j = i; j > 0; --j) at(j) = at(j - 1);
        at(0) = replacement;
        return true;
      }
    }
    return false;
  }
 private:
  T a_[N];
  size_t head_ = 0;
  size_t n_ = 0;
};
