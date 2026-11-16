#pragma once
#include <stddef.h>

// Newest-first, fixed storage. at(0) is the newest element.
template <typename T, size_t N>
class RingBuffer {
 public:
  void pushFront(const T& v) {
    size_t last = (n_ < N ? n_ : N - 1);
    for (size_t i = last; i > 0; --i) a_[i] = a_[i - 1];
    a_[0] = v;
    if (n_ < N) ++n_;
  }
  size_t size() const { return n_; }
  const T& at(size_t i) const { return a_[i]; }
  void clear() { n_ = 0; }
  // Replace the first match with `replacement` and move it to the front. False if none matched.
  template <class Pred>
  bool moveToFrontIf(Pred p, const T& replacement) {
    for (size_t i = 0; i < n_; ++i) {
      if (p(a_[i])) {
        for (size_t j = i; j > 0; --j) a_[j] = a_[j - 1];
        a_[0] = replacement;
        return true;
      }
    }
    return false;
  }
 private:
  T a_[N];
  size_t n_ = 0;
};
