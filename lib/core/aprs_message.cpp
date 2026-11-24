#include "aprs_message.h"
#include <string.h>
#include <ctype.h>

// Same rules as aprs_parse.cpp sanitizeComment (kept private there).
static void sanitizeText(const char* s, size_t len, char* out, size_t cap) {
  size_t n = 0;
  for (size_t i = 0; i < len && n + 1 < cap; i++) {
    unsigned char c = (unsigned char)s[i];
    if (c == '\r' || c == '\n' || c == '\t') c = ' ';
    if (c < 0x20 || c > 0x7E) continue;
    if (c == ' ' && n == 0) continue;
    out[n++] = (char)c;
  }
  while (n > 0 && out[n - 1] == ' ') n--;
  out[n] = 0;
}

// "ack"/"rej" followed by 1..5 alphanumerics and nothing else.
static bool isAckRej(const char* t) {
  if (strncmp(t, "ack", 3) && strncmp(t, "rej", 3)) return false;
  size_t n = strlen(t + 3);
  if (n < 1 || n > 5) return false;
  for (size_t i = 3; t[i]; i++) if (!isalnum((unsigned char)t[i])) return false;
  return true;
}

bool aprsParseMessage(const char* tnc2, const char* myCall, AprsMessage& out) {
  if (!tnc2 || !myCall || !*myCall) return false;
  const char* gt = strchr(tnc2, '>');
  if (!gt) return false;
  const char* info = strchr(gt, ':');
  if (!info) return false;
  info++;
  if (strlen(info) < 11 || info[0] != ':' || info[10] != ':') return false;

  // Addressee: 9 chars, space padded, compared case-insensitively with the SSID included.
  size_t an = 9;
  while (an > 0 && info[an] == ' ') an--;  // info[1..9] is the addressee
  if (an == 0 || an != strlen(myCall)) return false;
  for (size_t i = 0; i < an; i++)
    if (toupper((unsigned char)info[1 + i]) != toupper((unsigned char)myCall[i])) return false;

  const char* body = info + 11;
  size_t blen = strlen(body), tlen = blen, mlen = 0;
  const char* brace = strrchr(body, '{');
  if (brace && strlen(brace + 1) >= 1 && strlen(brace + 1) <= 5) {
    tlen = (size_t)(brace - body);
    mlen = strlen(brace + 1);
  }
  AprsMessage m;
  memset(&m, 0, sizeof(m));
  sanitizeText(body, tlen, m.text, sizeof(m.text));
  if (!m.text[0] || isAckRej(m.text)) return false;

  size_t n = 0;
  for (const char* p = tnc2; p < gt && n + 1 < sizeof(m.from); p++) {
    char c = (char)toupper((unsigned char)*p);
    if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-') m.from[n++] = c;
  }
  if (!m.from[0]) return false;
  if (mlen) sanitizeText(brace + 1, mlen, m.msgNo, sizeof(m.msgNo));
  m.sym[0] = m.sym[1] = ' ';
  m.distKm = -1;
  out = m;
  return true;
}

bool aprsMsgSame(const AprsMessage& a, const AprsMessage& b) {
  return !strcmp(a.from, b.from) && !strcmp(a.msgNo, b.msgNo) && !strcmp(a.text, b.text);
}
