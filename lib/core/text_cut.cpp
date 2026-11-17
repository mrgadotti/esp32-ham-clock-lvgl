#include "text_cut.h"

#include <stdio.h>
#include <string.h>

void textCut(char* out, size_t cap, const char* s, size_t maxBytes) {
  if (!cap) return;
  size_t n = strnlen(s, maxBytes);
  if (s[n])  // cut: back up to the start of the sequence the cut lands in
    while (n && ((uint8_t)s[n] & 0xC0) == 0x80) n--;
  if (n >= cap) {  // keep room for the terminator, again on a character boundary
    n = cap - 1;
    while (n && ((uint8_t)s[n] & 0xC0) == 0x80) n--;
  }
  memcpy(out, s, n);
  out[n] = 0;
}

void formatKmCell(char* out, size_t cap, int km) {
  if (km < 0) km = 0;
  if (km >= 10000) snprintf(out, cap, "%dk", km / 1000);
  else snprintf(out, cap, "%d", km);
}

void formatHmCell(char* out, size_t cap, uint32_t utc, int32_t offsetSec) {
  if (!utc) { snprintf(out, cap, "--"); return; }
  int64_t t = ((int64_t)utc + offsetSec) % 86400;
  if (t < 0) t += 86400;
  snprintf(out, cap, "%02u:%02u", (unsigned)(t / 3600), (unsigned)(t % 3600 / 60));
}
