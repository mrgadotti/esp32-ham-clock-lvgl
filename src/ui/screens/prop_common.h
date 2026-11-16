#pragma once
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "i18n.h"
#include "../theme.h"

// Good/Fair/Poor color (case-insensitive substring match, as in the original); grey otherwise.
inline lv_color_t propCondColor(const char* s) {
  char u[8];
  size_t i = 0;
  for (; s[i] && i < sizeof u - 1; i++) u[i] = toupper((unsigned char)s[i]);
  u[i] = 0;
  if (strstr(u, "GOOD")) return lv_color_hex(0x00FF00);
  if (strstr(u, "FAIR")) return lv_color_hex(0xFFFF00);
  if (strstr(u, "POOR")) return lv_color_hex(0xFF0000);
  return lv_color_hex(0xC0C0C0);
}

// Translated condition word for the XML value, or the raw text.
inline const char* propCondText(const char* s) {
  if (!strcasecmp(s, "Good")) return tr(Tr::Good);
  if (!strcasecmp(s, "Fair")) return tr(Tr::Fair);
  if (!strcasecmp(s, "Poor")) return tr(Tr::Poor);
  return s;
}

// "updated N min ago" from a Unix fetch time; empty if never fetched or the clock is not valid.
inline void propAgeText(uint32_t fetchedAt, char* out, size_t cap) {
  time_t now = time(nullptr);
  if (!fetchedAt || now < (time_t)fetchedAt) { out[0] = 0; return; }
  snprintf(out, cap, tr(Tr::UpdatedMinAgo), (int)((now - (time_t)fetchedAt) / 60));
}
