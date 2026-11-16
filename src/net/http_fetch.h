#pragma once
#include <ArduinoJson.h>
#include <stddef.h>
#include <stdint.h>

// Net task only. https uses setInsecure() (no CA pinning, as in the original).
// Body is NUL-terminated; fails cleanly if it does not fit in len-1 bytes.
bool httpGet(const char* url, char* buf, size_t len, uint32_t timeoutMs);
// Streaming parse; `filter` may be null.
bool httpGetJson(const char* url, JsonDocument& doc, const JsonDocument* filter);
// Streams a top-level JSON array one element at a time (only one element in RAM, for bodies too large for a
// document). `filter` applies to each element; `onItem` runs per element. Returns false on any parse error.
bool httpGetJsonArray(const char* url, const JsonDocument& filter, void (*onItem)(JsonObjectConst, void*), void* ctx);
