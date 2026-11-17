#pragma once
#include <stddef.h>
#include <stdint.h>

// Fixed ring of short boot-log lines. Not thread-safe: src/app/boot_log wraps it in a lock.
constexpr size_t kBootLogLines = 10, kBootLogLen = 40;  // 39 chars + NUL

struct BootLogRing {
  char line[kBootLogLines][kBootLogLen];
  uint8_t count;   // lines held (<= kBootLogLines)
  uint8_t next;    // slot the next push writes
  uint32_t ver;    // bumped on every push
  void push(const char* s);  // truncates (UTF-8 safe); control characters become spaces
  // i = 0 is the oldest held line, count - 1 the newest.
  const char* at(size_t i) const { return line[(next + kBootLogLines - count + i) % kBootLogLines]; }
};
