#pragma once
#include <ArduinoJson.h>
#include <stddef.h>
#include <stdint.h>

// One body buffer for every text fetch (hamqsl XML, QRZ). Net task only: use it one fetch at a time, and copy out
// what you need before the next httpGet.
constexpr size_t kHttpBodySize = 6144;
extern char httpBody[kHttpBodySize];

// Net task only. Call before a poll's first HTTPS fetch and return when it says false (nothing is consumed, ask again
// next tick): it waits while HamAlert holds the line and defers (2 s, at most 5 times) while the largest free block is
// too small for a TLS handshake.
bool httpPollGate(uint32_t nowMs);
// After a failed fetch: 4 s doubling to 60 s while SSL allocation keeps failing (reset by any 200), else 0 = normal interval.
uint32_t httpQuickRetryMs();

// Net task only. https uses setInsecure() (no CA pinning, as in the original).
// Body is NUL-terminated; fails cleanly if it does not fit in len-1 bytes.
bool httpGet(const char* url, char* buf, size_t len, uint32_t timeoutMs);
// Streaming parse; `filter` may be null.
bool httpGetJson(const char* url, JsonDocument& doc, const JsonDocument* filter);
// Streams a top-level JSON array one element at a time (only one element in RAM, for bodies too large for a
// document). `filter` applies to each element; `onItem` runs per element. Returns false on any parse error.
bool httpGetJsonArray(const char* url, const JsonDocument& filter, void (*onItem)(JsonObjectConst, void*), void* ctx);
