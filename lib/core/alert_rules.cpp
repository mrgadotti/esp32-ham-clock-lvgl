#include "alert_rules.h"
#include "aprs_wx.h"
#include <string.h>

// Uppercases and keeps only A-Z 0-9 - / (and '*' when star is set).
static void clean(const char* in, char* out, size_t cap, bool star) {
  size_t n = 0;
  for (; in && *in && n + 1 < cap; in++) {
    char c = *in;
    if (c >= 'a' && c <= 'z') c -= 32;
    if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '/' || (star && c == '*')) out[n++] = c;
  }
  out[n] = 0;
}

bool alertCallMatches(const char* watchToken, const char* call) {
  char w[16], c[16];
  clean(watchToken, w, sizeof w, true);
  clean(call, c, sizeof c, false);
  size_t wl = strlen(w), cl = strlen(c);
  if (!wl || !cl) return false;
  if (w[wl - 1] == '*') return wl > 1 && strncmp(w, c, wl - 1) == 0;
  if (strchr(w, '-')) return strcmp(w, c) == 0;
  return strncmp(w, c, wl) == 0 && (c[wl] == 0 || c[wl] == '-');
}

void AlertRules::configure(const AlertCfg& cfg) {
  cfg_ = cfg;
  nTokens_ = 0;
  const char* p = cfg.watchList;
  while (*p && nTokens_ < MAX_TOKENS) {
    char raw[32]; size_t n = 0;
    while (*p == ',' || *p == ' ' || *p == ';') p++;
    while (*p && *p != ',' && *p != ' ' && *p != ';') { if (n < sizeof raw - 1) raw[n++] = *p; p++; }
    raw[n] = 0;
    clean(raw, tokens_[nTokens_], TOK_LEN, true);
    if (tokens_[nTokens_][0]) nTokens_++;
  }
}

bool AlertRules::shouldAlert(const AprsStation& st, uint32_t now) {
  if (!cfg_.alertEnabled) return false;
  char call[12];
  clean(st.call, call, sizeof call, false);
  if (!call[0]) return false;

  bool hit = false;
  for (int i = 0; i < nTokens_ && !hit; i++) hit = alertCallMatches(tokens_[i], call);

  if (!hit && cfg_.rangeEnabled && st.hasLatLon && st.distKm >= 0 && st.distKm <= cfg_.alertKm) {
    const char* d = strrchr(call, '-');
    hit = d && d != call && (strcmp(d + 1, "7") == 0 || strcmp(d + 1, "9") == 0);
  }
  if (!hit && cfg_.wxEnabled && st.isWx) {
    AprsWx wx;  // note: st.comment is truncated to 47 chars, so late WX fields may be lost
    hit = aprsDecodeWx(st.comment, wx) && wx.hasTempC && wx.hasHumidity;
  }
  if (!hit) return false;

  uint32_t gap = (uint32_t)cfg_.minSeconds * 1000U;
  for (Slot& s : slots_) {
    if (s.used && strcmp(s.call, call) == 0) {
      if (gap && (uint32_t)(now - s.last) < gap) return false;
      s.last = now;
      return true;
    }
  }
  Slot* t = nullptr;
  for (Slot& s : slots_) if (!s.used) { t = &s; break; }
  if (!t) {  // full: prefer an expired slot, else evict the oldest
    for (Slot& s : slots_) if ((uint32_t)(now - s.last) >= gap) { t = &s; break; }
    if (!t) {
      t = &slots_[0];
      for (Slot& s : slots_) if ((uint32_t)(now - s.last) > (uint32_t)(now - t->last)) t = &s;
    }
  }
  strcpy(t->call, call); t->last = now; t->used = true;
  return true;
}
