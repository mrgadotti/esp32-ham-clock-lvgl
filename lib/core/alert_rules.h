#pragma once
#include "types.h"
// token: callsign, callsign-SSID (exact), or prefix ending in '*'. A token without SSID matches any SSID.
// Both arguments are case-insensitive.
bool alertCallMatches(const char* watchToken, const char* call);

class AlertRules {
 public:
  void configure(const AlertCfg& cfg);  // tokenizes the watch list once; keeps the cooldown table
  // now: monotonic milliseconds (e.g. millis()). Wrap-safe. alertEnabled is the master switch.
  bool shouldAlert(const AprsStation& st, uint32_t now);
 private:
  static const int MAX_TOKENS = 16, TOK_LEN = 14, COOLDOWN_SLOTS = 16;
  AlertCfg cfg_{};
  char tokens_[MAX_TOKENS][TOK_LEN] = {};
  int nTokens_ = 0;
  struct Slot { char call[12]; uint32_t last; bool used; } slots_[COOLDOWN_SLOTS] = {};
  int replaceIdx_ = 0;
};
