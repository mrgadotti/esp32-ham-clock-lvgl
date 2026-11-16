#include "net/http_fetch.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <string.h>

constexpr uint32_t HTTP_TIMEOUT_MS = 8000;

// Runs `body(http)` on a 200 response. Logs the host only: query strings carry credentials.
template <class F>
static bool fetch(const char* url, uint32_t timeoutMs, F body) {
  bool https = strncmp(url, "https://", 8) == 0;
  WiFiClientSecure sec;
  WiFiClient plain;
  if (https) sec.setInsecure();
  HTTPClient http;
  http.setTimeout(timeoutMs);
  http.setConnectTimeout(timeoutMs);
  uint32_t t0 = millis();
  if (!http.begin(https ? static_cast<WiFiClient&>(sec) : plain, url)) return false;
  int code = http.GET();
  const char* h = strstr(url, "://");
  h = h ? h + 3 : url;
  Serial.printf("[HTTP] GET %.*s -> %d (%u ms)\n", (int)strcspn(h, "/?:;"), h, code, (unsigned)(millis() - t0));
  bool ok = code == 200 && body(http);
  http.end();
  return ok;
}

bool httpGet(const char* url, char* buf, size_t len, uint32_t timeoutMs) {
  if (!buf || len < 2) return false;
  return fetch(url, timeoutMs, [&](HTTPClient& http) {
    int sz = http.getSize();  // -1 when chunked
    if (sz >= 0 && (size_t)sz > len - 1) return false;
    WiFiClient& s = http.getStream();
    size_t got = 0;
    uint32_t last = millis();
    while (got < len - 1 && (http.connected() || s.available()) && millis() - last < timeoutMs) {
      int a = s.available();
      if (a <= 0) { delay(1); continue; }
      if ((size_t)a > len - 1 - got) a = len - 1 - got;
      got += s.readBytes(buf + got, a);
      last = millis();
      if (sz >= 0 && got >= (size_t)sz) break;
    }
    buf[got] = 0;
    if (sz >= 0) return got == (size_t)sz;
    return got < len - 1 || !s.available();  // chunked: a full buffer with more pending is an overflow
  });
}

bool httpGetJson(const char* url, JsonDocument& doc, const JsonDocument* filter) {
  return fetch(url, HTTP_TIMEOUT_MS, [&](HTTPClient& http) {
    DeserializationError e = filter ? deserializeJson(doc, http.getStream(), DeserializationOption::Filter(*filter))
                                    : deserializeJson(doc, http.getStream());
    if (e) Serial.printf("[HTTP] json: %s\n", e.c_str());
    return !e;
  });
}
