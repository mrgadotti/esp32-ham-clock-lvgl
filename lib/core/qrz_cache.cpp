#include "qrz_cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool elapsed(uint32_t now, uint32_t t) { return (int32_t)(now - t) >= 0; }

bool qrzNormalizeCall(const char* in, char out[QRZ_CALL_LEN]) {
  if (!in) return false;
  size_t n = 0;
  for (; *in && n < QRZ_CALL_LEN - 1; in++) {
    char c = *in;
    if (c >= 'a' && c <= 'z') c -= 32;
    if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '/' || c == '-') out[n++] = c;
  }
  out[n] = 0;
  return n > 0;
}

bool QrzCache::get(const char* call, uint32_t now, QrzInfo& out) const {
  char c[QRZ_CALL_LEN];
  if (!qrzNormalizeCall(call, c)) return false;
  for (const Entry& e : e_)
    if (e.used && !strcmp(e.call, c) && (uint32_t)(now - e.at) < QRZ_CACHE_TTL_MS) { out = e.info; return true; }
  return false;
}

void QrzCache::put(const char* call, const QrzInfo& info, uint32_t now) {
  char c[QRZ_CALL_LEN];
  if (!qrzNormalizeCall(call, c)) return;
  int pick = -1;
  for (int i = 0; i < QRZ_CACHE_SIZE; i++) {
    if (e_[i].used && !strcmp(e_[i].call, c)) { pick = i; break; }
    if (pick < 0) { pick = i; continue; }
    if (!e_[pick].used) continue;
    if (!e_[i].used || (uint32_t)(now - e_[i].at) > (uint32_t)(now - e_[pick].at)) pick = i;
  }
  e_[pick].used = true;
  strcpy(e_[pick].call, c);
  e_[pick].info = info;
  e_[pick].at = now;
}

int QrzQueue::find(const char* c) const {
  for (int i = 0; i < n_; i++) if (!strcmp(q_[i].call, c)) return i;
  return -1;
}

void QrzQueue::removeAt(int i) {
  for (; i < n_ - 1; i++) q_[i] = q_[i + 1];
  n_--;
}

bool QrzQueue::contains(const char* call) const {
  char c[QRZ_CALL_LEN];
  return qrzNormalizeCall(call, c) && find(c) >= 0;
}

bool QrzQueue::enqueue(const char* call, uint32_t now) {
  char c[QRZ_CALL_LEN];
  if (!qrzNormalizeCall(call, c) || n_ >= QRZ_QUEUE_SIZE || find(c) >= 0) return false;
  strcpy(q_[n_].call, c);
  q_[n_].attempts = 0;
  q_[n_].nextTry = now;
  n_++;
  return true;
}

bool QrzQueue::next(uint32_t now, char out[QRZ_CALL_LEN]) {
  if (started_ && (uint32_t)(now - lastMs_) < QRZ_INTERVAL_MS) return false;
  for (int i = 0; i < n_; i++)
    if (elapsed(now, q_[i].nextTry)) {
      strcpy(out, q_[i].call);
      lastMs_ = now;
      started_ = true;
      return true;
    }
  return false;
}

void QrzQueue::done(const char* call) {
  char c[QRZ_CALL_LEN];
  if (!qrzNormalizeCall(call, c)) return;
  int i = find(c);
  if (i >= 0) removeAt(i);
}

void QrzQueue::fail(const char* call, uint32_t now) {
  char c[QRZ_CALL_LEN];
  if (!qrzNormalizeCall(call, c)) return;
  int i = find(c);
  if (i < 0) return;
  if (++q_[i].attempts >= QRZ_RETRY_LIMIT) removeAt(i);
  else q_[i].nextTry = now + QRZ_RETRY_DELAY_MS;
}

static bool isWs(char c) { return c == ' ' || c == '\n' || c == '\r' || c == '\t'; }

// Copies the trimmed text of <tag>...</tag>; false if absent, empty or too long.
static bool tagValue(const char* xml, const char* tag, char* out, size_t cap) {
  char open[24], close[24];
  if (!xml || strlen(tag) > 16) return false;
  snprintf(open, sizeof open, "<%s>", tag);
  snprintf(close, sizeof close, "</%s>", tag);
  const char* s = strstr(xml, open);
  if (!s) return false;
  s += strlen(open);
  const char* e = strstr(s, close);
  if (!e) return false;
  while (s < e && isWs(*s)) s++;
  while (e > s && isWs(e[-1])) e--;
  size_t n = e - s;
  if (n == 0 || n >= cap) return false;
  memcpy(out, s, n);
  out[n] = 0;
  return true;
}

bool qrzParseSessionKey(const char* xml, char* key, size_t cap) {
  char tmp[64];
  if (!xml || !key || strstr(xml, "<Error>")) return false;
  if (!tagValue(xml, "Key", tmp, sizeof tmp) || strlen(tmp) >= cap) return false;
  strcpy(key, tmp);
  return true;
}

bool qrzParseXml(const char* xml, QrzInfo& out) {
  char v[32], lo[32];
  if (!xml || strstr(xml, "<Error>")) return false;
  QrzInfo r = {};
  bool g = tagValue(xml, "grid", v, sizeof v) && strlen(v) < sizeof r.grid;
  if (g) strcpy(r.grid, v);
  bool c = (tagValue(xml, "country", v, sizeof v) || tagValue(xml, "dxcc", v, sizeof v)) && strlen(v) < sizeof r.country;
  if (c) strcpy(r.country, v);
  if (!g && !c) return false;
  if (tagValue(xml, "lat", v, sizeof v) && tagValue(xml, "lon", lo, sizeof lo)) {
    r.lat = (float)atof(v);
    r.lon = (float)atof(lo);
    r.hasLatLon = true;
  }
  out = r;
  return true;
}
