#include "propagation_xml.h"
#include <stdio.h>
#include <string.h>

// Copies the trimmed text between `open` and `close` into dst; returns false if absent or empty.
static bool extractBetween(const char* xml, const char* open, const char* close, char* dst, size_t cap) {
  const char* s = strstr(xml, open);
  if (!s) return false;
  s += strlen(open);
  const char* e = strstr(s, close);
  if (!e) return false;
  while (s < e && (*s == ' ' || *s == '\n' || *s == '\r' || *s == '\t')) s++;
  while (e > s && (e[-1] == ' ' || e[-1] == '\n' || e[-1] == '\r' || e[-1] == '\t')) e--;
  size_t n = 0;
  char tmp[32];  // all fields are <= 24 bytes, so cap-1 < sizeof tmp
  for (; s < e && n < cap - 1; s++)
    if ((unsigned char)*s >= 0x20 && (unsigned char)*s <= 0x7E) tmp[n++] = *s;  // printable ASCII only
  if (n == 0) return false;
  memcpy(dst, tmp, n);
  dst[n] = 0;
  return true;
}

static bool extractTag(const char* xml, const char* tag, char* dst, size_t cap) {
  char open[24], close[24];
  snprintf(open, sizeof open, "<%s>", tag);
  snprintf(close, sizeof close, "</%s>", tag);
  return extractBetween(xml, open, close, dst, cap);
}

bool parsePropagationXml(const char* xml, PropagationData& out) {
  static const char* const labels[4] = {"80m-40m", "30m-20m", "17m-15m", "12m-10m"};
  if (!xml) return false;
  PropagationData p = out;  // work on a copy so failure leaves `out` untouched
  if (!extractTag(xml, "solarflux", p.sfi, sizeof p.sfi) || !extractTag(xml, "kindex", p.kindex, sizeof p.kindex))
    return false;
  if (!extractTag(xml, "aindex", p.aindex, sizeof p.aindex)) strcpy(p.aindex, "--");
  if (!extractTag(xml, "muf", p.muf, sizeof p.muf)) strcpy(p.muf, "--");
  if (!extractTag(xml, "updated", p.updated, sizeof p.updated)) strcpy(p.updated, "--");
  for (int i = 0; i < 4; i++) {
    strcpy(p.band[i].label, labels[i]);
    strcpy(p.band[i].day, "--");
    strcpy(p.band[i].night, "--");
    char open[48];
    snprintf(open, sizeof open, "<band name=\"%s\" time=\"day\">", labels[i]);
    extractBetween(xml, open, "</band>", p.band[i].day, sizeof p.band[i].day);
    snprintf(open, sizeof open, "<band name=\"%s\" time=\"night\">", labels[i]);
    extractBetween(xml, open, "</band>", p.band[i].night, sizeof p.band[i].night);
  }
  p.valid = true;
  out = p;
  return true;
}
