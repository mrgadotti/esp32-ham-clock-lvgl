#pragma once
#include <ArduinoJson.h>
#include <stddef.h>
#include <stdint.h>

// Net task only. https uses setInsecure() (no CA pinning, as in the original).
// Body is NUL-terminated; fails cleanly if it does not fit in len-1 bytes.
bool httpGet(const char* url, char* buf, size_t len, uint32_t timeoutMs);
// Streaming parse; `filter` may be null.
bool httpGetJson(const char* url, JsonDocument& doc, const JsonDocument* filter);
