#include "net/http_fetch.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <string.h>

#include "app/mem_log.h"

constexpr uint32_t HTTP_TIMEOUT_MS = 8000;

char httpBody[kHttpBodySize];

// Runs `body(http, ctx)` on a 200 response. Logs the host only: query strings carry credentials.
// Non-template so the HTTPClient + TLS setup exists once; fetchWith() below adapts lambdas.
static bool fetch(const char* url, uint32_t timeoutMs, bool (*body)(HTTPClient&, void*), void* ctx) {
  bool https = strncmp(url, "https://", 8) == 0;
  WiFiClientSecure sec;
  WiFiClient plain;
  if (https) sec.setInsecure();
  HTTPClient http;
  http.setTimeout(timeoutMs);
  http.setConnectTimeout(timeoutMs);
  http.useHTTP10(true);  // no chunked encoding, body is close-delimited
  http.setReuse(false);
  uint32_t t0 = millis();
  uint32_t big0 = memLargest8();
  if (!http.begin(https ? static_cast<WiFiClient&>(sec) : plain, url)) return false;
  int code = http.GET();
  const char* h = strstr(url, "://");
  h = h ? h + 3 : url;
  Serial.printf("[HTTP] GET %.*s -> %d (%u ms) largest=%u->%u free=%u\n", (int)strcspn(h, "/?:;"), h, code, (unsigned)(millis() - t0),
                (unsigned)big0, (unsigned)memLargest8(), (unsigned)ESP.getFreeHeap());
  bool ok = code == 200 && body(http, ctx);
  http.end();
  return ok;
}

template <class F>
static bool fetchWith(const char* url, uint32_t timeoutMs, F body) {
  return fetch(url, timeoutMs, [](HTTPClient& h, void* p) { return (*static_cast<F*>(p))(h); }, &body);
}

bool httpGet(const char* url, char* buf, size_t len, uint32_t timeoutMs) {
  if (!buf || len < 2) return false;
  return fetchWith(url, timeoutMs, [&](HTTPClient& http) {
    int sz = http.getSize();  // -1 when chunked
    if (sz >= 0 && (size_t)sz > len - 1) return false;
    Stream& s = http.getStream();
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
    if (got < len - 1) return true;
    // Buffer full without Content-Length: any further byte before close means overflow.
    while (http.connected() && !s.available() && millis() - last < timeoutMs) delay(1);
    return !s.available() && !http.connected();  // still open after timeout: body may be oversized
  });
}

bool httpGetJson(const char* url, JsonDocument& doc, const JsonDocument* filter) {
  return fetchWith(url, HTTP_TIMEOUT_MS, [&](HTTPClient& http) {
    Stream& s = http.getStream();  // typed Stream&: one ArduinoJson reader instantiation shared with httpGetJsonArray
    DeserializationError e = filter ? deserializeJson(doc, s, DeserializationOption::Filter(*filter)) : deserializeJson(doc, s);
    if (e) Serial.printf("[HTTP] json: %s\n", e.c_str());
    return !e;
  });
}

bool httpGetJsonArray(const char* url, const JsonDocument& filter, void (*onItem)(JsonObjectConst, void*), void* ctx) {
  return fetchWith(url, HTTP_TIMEOUT_MS, [&](HTTPClient& http) {
    Stream& s = http.getStream();
    if (!s.find('[')) return false;
    size_t n = 0;
    do {
      JsonDocument elem;
      DeserializationError e = deserializeJson(elem, s, DeserializationOption::Filter(filter));
      if (e) {
        Serial.printf("[HTTP] json item %u: %s\n", (unsigned)n, e.c_str());
        return false;
      }
      onItem(elem.as<JsonObjectConst>(), ctx);
      n++;
    } while (s.findUntil(",", "]"));
    Serial.printf("[HTTP] json array: %u items\n", (unsigned)n);
    return true;
  });
}
