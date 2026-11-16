#include "ax25_kiss.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static const uint8_t FEND = 0xC0, FESC = 0xDB, TFEND = 0xDC, TFESC = 0xDD;

static bool putAddr(uint8_t* b, size_t cap, size_t& off, const char* tok, bool last) {
  if (!tok || off + 7 > cap) return false;
  char base[7];
  size_t n = 0;
  while (tok[n] && tok[n] != '-') {
    if (n >= 6 || !isalnum((unsigned char)tok[n])) return false;
    base[n] = (char)toupper((unsigned char)tok[n]);
    n++;
  }
  if (n == 0) return false;
  int ssid = 0;
  if (tok[n] == '-') {
    const char* s = tok + n + 1;
    if (!*s) return false;
    for (; *s; s++) {
      if (!isdigit((unsigned char)*s)) return false;
      ssid = ssid * 10 + (*s - '0');
      if (ssid > 15) return false;
    }
  }
  for (size_t i = 0; i < 6; i++) b[off + i] = (uint8_t)((i < n ? base[i] : ' ') << 1);
  b[off + 6] = (uint8_t)(0x60 | (ssid << 1) | (last ? 1 : 0));
  off += 7;
  return true;
}

size_t kissEncodeUiFrame(const char* src, const char* dst, const char* const* path, uint8_t nPath,
                         const char* info, uint8_t* out, size_t cap) {
  uint8_t ax[360];
  size_t n = 0;
  if (!info || !out || nPath > 8) return 0;
  if (!putAddr(ax, sizeof ax, n, dst, false)) return 0;
  if (!putAddr(ax, sizeof ax, n, src, nPath == 0)) return 0;
  for (uint8_t i = 0; i < nPath; i++)
    if (!putAddr(ax, sizeof ax, n, path[i], i == nPath - 1)) return 0;
  size_t il = strlen(info);
  if (n + 2 + il > sizeof ax) return 0;
  ax[n++] = 0x03;
  ax[n++] = 0xF0;
  memcpy(ax + n, info, il);
  n += il;

  size_t o = 0;
  if (cap < 3) return 0;
  out[o++] = FEND;
  out[o++] = 0x00;
  for (size_t i = 0; i < n; i++) {
    uint8_t c = ax[i];
    size_t need = (c == FEND || c == FESC) ? 2 : 1;
    if (o + need + 1 > cap) return 0;  // +1 for closing FEND
    if (c == FEND) { out[o++] = FESC; out[o++] = TFEND; }
    else if (c == FESC) { out[o++] = FESC; out[o++] = TFESC; }
    else out[o++] = c;
  }
  out[o++] = FEND;
  return o;
}

bool kissDeframe(uint8_t b, KissRx& st) {
  if (st.done) { st.done = false; st.len = 0; st.haveCmd = false; st.esc = false; }
  if (b == FEND) {
    bool complete = st.inFrame && st.len > 0 && !st.esc && (st.cmd & 0x0F) == 0;
    st.inFrame = true;
    st.esc = false;
    if (complete) { st.done = true; return true; }
    st.len = 0;
    st.haveCmd = false;
    return false;
  }
  if (!st.inFrame) return false;
  if (st.esc) {
    st.esc = false;
    if (b == TFEND) b = FEND; else if (b == TFESC) b = FESC;
  } else if (b == FESC) {
    st.esc = true;
    return false;
  }
  if (!st.haveCmd) { st.cmd = b; st.haveCmd = true; return false; }
  if (st.len >= sizeof st.buf) { st.inFrame = false; st.len = 0; st.haveCmd = false; return false; }
  st.buf[st.len++] = b;
  return false;
}

// Appends "CALL[-SSID][*]" at out[o]; returns false on overflow.
static bool addrToText(const uint8_t* a, bool digi, char* out, size_t cap, size_t& o) {
  char t[16];
  size_t n = 0;
  for (int i = 0; i < 6; i++) {
    char c = (char)(a[i] >> 1);
    if (c != ' ') t[n++] = c;
  }
  if (n == 0) return false;
  for (size_t i = 0; i < n; i++) if (!isalnum((unsigned char)t[i])) return false;
  int ssid = (a[6] >> 1) & 0x0F;
  if (ssid) n += (size_t)snprintf(t + n, sizeof t - n, "-%d", ssid);
  if (digi && (a[6] & 0x80)) t[n++] = '*';
  if (o + n >= cap) return false;
  memcpy(out + o, t, n);
  o += n;
  return true;
}

bool ax25ToTnc2(const uint8_t* f, size_t len, char* dst, size_t dstCap) {
  if (!f || !dst || dstCap == 0 || len < 16) return false;
  char out[640];  // built locally so dst is untouched on failure
  size_t cap = dstCap < sizeof out ? dstCap : sizeof out;
  size_t o = 0, off = 14;
  bool last = f[13] & 1;
  if (!addrToText(f + 7, false, out, cap, o)) return false;       // source
  if (o + 1 >= cap) return false;
  out[o++] = '>';
  if (!addrToText(f, false, out, cap, o)) return false;           // destination
  for (int g = 0; !last; g++) {
    if (g >= 8 || off + 7 > len) return false;
    if (o + 1 >= cap) return false;
    out[o++] = ',';
    if (!addrToText(f + off, true, out, cap, o)) return false;
    last = f[off + 6] & 1;
    off += 7;
  }
  if (off < len && f[off] == 0x03) off++;
  if (off < len && f[off] == 0xF0) off++;
  if (off >= len || o + 1 >= cap) return false;
  out[o++] = ':';
  size_t infoStart = o;
  for (; off < len; off++) {
    if (f[off] < 0x20 || f[off] > 0x7E) continue;
    if (o + 1 >= cap) return false;
    out[o++] = (char)f[off];
  }
  if (o == infoStart) return false;
  out[o] = 0;
  memcpy(dst, out, o + 1);
  return true;
}

bool formatAprsPosition(double lat, double lon, char symTable, char symCode, const char* comment,
                        char* out, size_t cap) {
  if (!out || cap == 0 || !(lat >= -90.0 && lat <= 90.0) || !(lon >= -180.0 && lon <= 180.0)) return false;
  // hundredths of a minute, rounded once so 59.999' carries into the degree
  long la = lround(fabs(lat) * 6000.0), lo = lround(fabs(lon) * 6000.0);
  char tmp[256];  // comments that do not fit fail; out stays untouched
  int n = snprintf(tmp, sizeof tmp,"!%02ld%02ld.%02ld%c%c%03ld%02ld.%02ld%c%c%s",
                   la / 6000, (la % 6000) / 100, la % 100, lat >= 0 ? 'N' : 'S', symTable,
                   lo / 6000, (lo % 6000) / 100, lo % 100, lon >= 0 ? 'E' : 'W', symCode,
                   comment ? comment : "");
  if (n <= 0 || (size_t)n >= sizeof tmp || (size_t)n >= cap) return false;
  memcpy(out, tmp, (size_t)n + 1);
  return true;
}

bool aprsTxCallValid(const char* call) {
  if (!call) return false;
  while (isspace((unsigned char)*call)) call++;
  char c[16];
  size_t n = 0;
  for (; call[n] && call[n] != '-' && !isspace((unsigned char)call[n]); n++) {
    if (n >= sizeof c - 1) return false;
    c[n] = (char)toupper((unsigned char)call[n]);
  }
  c[n] = 0;
  return n > 0 && strcmp(c, "NOCALL") && strcmp(c, "N0CALL");
}
