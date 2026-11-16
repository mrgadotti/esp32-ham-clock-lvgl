#pragma once
#include <stddef.h>
#include <stdint.h>
#include "types.h"

// All times are injected millis(); comparisons are wrap-safe.
constexpr uint8_t QRZ_CACHE_SIZE = 20;
constexpr uint8_t QRZ_QUEUE_SIZE = 20;
constexpr uint32_t QRZ_CACHE_TTL_MS = 15UL * 60UL * 1000UL;
constexpr uint32_t QRZ_INTERVAL_MS = 3000;
constexpr uint32_t QRZ_RETRY_DELAY_MS = 5000;
constexpr uint8_t QRZ_RETRY_LIMIT = 2;
constexpr size_t QRZ_CALL_LEN = 12;

// Uppercase, keep [A-Z0-9/-], max 11 chars. False if nothing is left.
bool qrzNormalizeCall(const char* in, char out[QRZ_CALL_LEN]);

class QrzCache {
 public:
  bool get(const char* call, uint32_t now, QrzInfo& out) const;
  void put(const char* call, const QrzInfo& info, uint32_t now);  // replaces same call, else oldest
 private:
  struct Entry { bool used; char call[QRZ_CALL_LEN]; QrzInfo info; uint32_t at; };
  Entry e_[QRZ_CACHE_SIZE] = {};
};

class QrzQueue {
 public:
  bool enqueue(const char* call, uint32_t now);  // false: invalid, duplicate or full
  bool contains(const char* call) const;
  // Next due call, honouring the 3 s spacing; marks the attempt time.
  bool next(uint32_t now, char out[QRZ_CALL_LEN]);
  void done(const char* call);                   // success: remove
  void fail(const char* call, uint32_t now);     // retry in 5 s, dropped after 2 attempts
  uint8_t size() const { return n_; }
 private:
  struct Item { char call[QRZ_CALL_LEN]; uint8_t attempts; uint32_t nextTry; };
  int find(const char* normalized) const;
  void removeAt(int i);
  Item q_[QRZ_QUEUE_SIZE] = {};
  uint8_t n_ = 0;
  uint32_t lastMs_ = 0;
  bool started_ = false;
};

// Both return false and leave outputs untouched on garbage / <Error>.
bool qrzParseXml(const char* xml, QrzInfo& out);
bool qrzParseSessionKey(const char* xml, char* key, size_t cap);
