#include "aprs_parse.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

static bool symTableCompressed(char c) { return c == '/' || c == '\\' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'j'); }
static bool symTableUncompressed(char c) { return c == '/' || c == '\\' || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'); }

static bool fixedUnsigned(const char* s, int off, int n, unsigned& out) {
  out = 0;
  for (int i = 0; i < n; i++) {
    if (s[off + i] < '0' || s[off + i] > '9') return false;
    out = out * 10U + (unsigned)(s[off + i] - '0');
  }
  return true;
}

static bool latLonOk(float lat, float lon) {
  if (isnan(lat) || isnan(lon) || lat < -90 || lat > 90 || lon < -180 || lon > 180) return false;
  return !(fabsf(lat) < 0.0001f && fabsf(lon) < 0.0001f);  // 0,0 means "no position"
}

static int micEBodyOffset(const unsigned char* body, size_t len) {
  const size_t fixed = 8;
  if (len <= fixed) return (int)fixed;
  size_t idx = fixed;
  unsigned char m = body[idx];
  if (m == 0x60 || m == 0x27 || m == 0x1C || m == 0x1D) {
    size_t probe = idx + 1, n = 0;
    while (probe < len && n < 10 && body[probe] >= 0x21 && body[probe] <= 0x7B) { n++; probe++; }
    if (n >= 2) idx = probe;
  }
  return (int)idx;
}

static bool parseUncompressed(const char* body, size_t len, float& lat, float& lon, char& tbl, char& code) {
  if (len < 19) return false;
  char p[20];
  memcpy(p, body, 19); p[19] = 0;
  // position ambiguity: spaces become the middle of the range
  if (p[2] == ' ') p[2] = '3';
  if (p[3] == ' ') p[3] = '5';
  if (p[5] == ' ') p[5] = '5';
  if (p[6] == ' ') p[6] = '5';
  if (p[12] == ' ') p[12] = '3';
  if (p[13] == ' ') p[13] = '5';
  if (p[15] == ' ') p[15] = '5';
  if (p[16] == ' ') p[16] = '5';
  if (p[4] != '.' || p[14] != '.') return false;
  unsigned latD, latM, latF, lonD, lonM, lonF;
  if (!fixedUnsigned(p, 0, 2, latD) || !fixedUnsigned(p, 2, 2, latM) || !fixedUnsigned(p, 5, 2, latF) ||
      !fixedUnsigned(p, 9, 3, lonD) || !fixedUnsigned(p, 12, 2, lonM) || !fixedUnsigned(p, 15, 2, lonF)) return false;
  char lh = p[7], gh = p[17];
  tbl = p[8]; code = p[18];
  if (!symTableUncompressed(tbl)) tbl = 0;
  bool south = lh == 'S' || lh == 's', west = gh == 'W' || gh == 'w';
  if (!south && lh != 'N' && lh != 'n') return false;
  if (!west && gh != 'E' && gh != 'e') return false;
  if (latD > 90 || lonD > 180 || latM >= 60 || lonM >= 60) return false;
  lat = latD + latM / 60.0f + latF / 6000.0f;
  lon = lonD + lonM / 60.0f + lonF / 6000.0f;
  if (south) lat = -lat;
  if (west) lon = -lon;
  return latLonOk(lat, lon);
}

static bool parseCompressed(const char* b, size_t len, float& lat, float& lon, char& tbl, char& code) {
  if (len < 13) return false;
  tbl = b[0]; code = b[9];
  if (!symTableCompressed(tbl)) return false;
  for (int i = 1; i <= 8; i++) if (b[i] < 0x21 || b[i] > 0x7b) return false;
  int la = 0, lo = 0;
  for (int i = 1; i <= 4; i++) la = la * 91 + (b[i] - 33);
  for (int i = 5; i <= 8; i++) lo = lo * 91 + (b[i] - 33);
  lat = 90.0f - la / 380926.0f;
  lon = -180.0f + lo / 190463.0f;
  if (tbl >= 'a' && tbl <= 'j') tbl -= 81;  // a-j -> 0-9
  return latLonOk(lat, lon);
}

static bool parseMicE(const char* dstcall, const unsigned char* body, size_t len,
                      float& lat, float& lon, char& tbl, char& code) {
  if (!dstcall || strlen(dstcall) != 6 || len < 8) return false;
  for (int i = 0; i < 3; i++) {
    char c = dstcall[i];
    if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'L') || (c >= 'P' && c <= 'Z'))) return false;
  }
  for (int i = 3; i < 6; i++) {
    char c = dstcall[i];
    if (!((c >= '0' && c <= '9') || c == 'L' || (c >= 'P' && c <= 'Z'))) return false;
  }
  if (body[0] < 0x26 || body[0] > 0x7f || body[1] < 0x26 || body[1] > 0x61) return false;
  if (body[2] < 0x1c || body[3] < 0x1c || body[4] < 0x1c || body[5] < 0x1c || body[4] > 0x7d) return false;
  if ((body[6] < 0x21 || body[6] > 0x7b) && body[6] != 0x7d) return false;
  if (body[7] != '/' && body[7] != '\\') return false;

  char d[7];
  memcpy(d, dstcall, 6); d[6] = 0;
  for (char* p = d; *p; p++) {
    if (*p >= 'A' && *p <= 'J') *p -= 'A' - '0';
    else if (*p >= 'P' && *p <= 'Y') *p -= 'P' - '0';
    else if (*p == 'K' || *p == 'L' || *p == 'Z') *p = '_';
  }
  int amb = 0;
  if (d[5] == '_') { d[5] = '5'; amb = 1; }
  if (d[4] == '_') { d[4] = '5'; amb = 2; }
  if (d[3] == '_') { d[3] = '5'; amb = 3; }
  if (d[2] == '_') { d[2] = '3'; amb = 4; }
  if (d[1] == '_' || d[0] == '_') return false;
  unsigned latD, latM, latF;
  if (!fixedUnsigned(d, 0, 2, latD) || !fixedUnsigned(d, 2, 2, latM) || !fixedUnsigned(d, 4, 2, latF)) return false;
  lat = latD + latM / 60.0f + latF / 6000.0f;
  if (dstcall[3] <= 0x4c) lat = -lat;

  unsigned lonD = body[0] - 28;
  if (dstcall[4] >= 0x50) lonD += 100;
  if (lonD >= 180 && lonD <= 189) lonD -= 80;
  else if (lonD >= 190 && lonD <= 199) lonD -= 190;
  unsigned lonM = body[1] - 28;
  if (lonM >= 60) lonM -= 60;
  unsigned lonF = body[2] - 28;
  switch (amb) {
    case 0: lon = lonD + lonM / 60.0f + lonF / 6000.0f; break;
    case 1: lon = lonD + lonM / 60.0f + (lonF - lonF % 10 + 5) / 6000.0f; break;
    case 2: lon = lonD + (lonM + 0.5f) / 60.0f; break;
    case 3: lon = lonD + (lonM - lonM % 10 + 5) / 60.0f; break;
    default: lon = lonD + 0.5f; break;
  }
  if (dstcall[5] >= 0x50) lon = -lon;
  code = (char)body[6];
  tbl = (char)body[7];
  return latLonOk(lat, lon);
}

// Position at pos (compressed or uncompressed); on success *consumed = bytes used.
static bool parsePos(const char* pos, size_t len, float& lat, float& lon, char& tbl, char& code, int& consumed) {
  if (len == 0) return false;
  if (symTableCompressed(pos[0])) { consumed = 13; return parseCompressed(pos, len, lat, lon, tbl, code); }
  if (pos[0] >= '0' && pos[0] <= '9') { consumed = 19; return parseUncompressed(pos, len, lat, lon, tbl, code); }
  return false;
}

static void sanitizeComment(const char* s, char* out, size_t cap) {
  size_t n = 0;
  for (; *s && n + 1 < cap; s++) {
    unsigned char c = (unsigned char)*s;
    if (c == '\r' || c == '\n' || c == '\t') c = ' ';
    if (c < 0x20 || c > 0x7E) continue;
    if (c == ' ' && n == 0) continue;
    out[n++] = (char)c;
  }
  while (n > 0 && out[n - 1] == ' ') n--;
  out[n] = 0;
}

bool aprsExtractFreq(const char* text, float& mhz) {
  if (!text) return false;
  int len = (int)strlen(text);
  for (int i = 0; i < len; i++) {
    if (!(text[i] >= '0' && text[i] <= '9') || (i > 0 && text[i - 1] >= '0' && text[i - 1] <= '9')) continue;
    int j = i;
    bool dot = false;
    while (j < len) {
      if (text[j] == '.') { if (dot) break; dot = true; j++; continue; }
      if (text[j] < '0' || text[j] > '9') break;
      j++;
    }
    char num[16];
    int n = j - i < 15 ? j - i : 15;
    memcpy(num, text + i, n); num[n] = 0;
    float v = (float)atof(num);
    if (v <= 0.0f) continue;
    int k = j;
    while (k < len && (text[k] == ' ' || text[k] == '\t')) k++;
    if (k + 2 < len && (text[k] | 0x20) == 'm' && (text[k + 1] | 0x20) == 'h' && (text[k + 2] | 0x20) == 'z') { mhz = v; return true; }
    if (dot && v >= 30.0f && v <= 1000.0f) { mhz = v; return true; }
  }
  return false;
}

bool aprsParseFrame(const char* line, AprsStation& st) {
  st = AprsStation();
  st.distKm = -1; st.bearing = -1;
  if (!line) return false;
  const char* gt = strchr(line, '>');
  if (!gt || gt == line) return false;
  const char* colon = strchr(gt, ':');
  if (!colon) return false;

  // source call (trimmed, truncated)
  const char* s = line; const char* e = gt;
  while (s < e && *s == ' ') s++;
  while (e > s && e[-1] == ' ') e--;
  if (e == s) return false;
  size_t cl = (size_t)(e - s); if (cl > sizeof(st.call) - 1) cl = sizeof(st.call) - 1;
  memcpy(st.call, s, cl); st.call[cl] = 0;

  // destination call
  char dst[16];
  size_t dl = 0;
  for (const char* p = gt + 1; p < colon && *p != ',' && dl < sizeof(dst) - 1; p++) dst[dl++] = *p;
  dst[dl] = 0;

  const char* payload = colon + 1;
  size_t plen = strlen(payload);
  if (plen == 0) return false;
  char type = payload[0];
  const char* body = payload + 1;
  size_t blen = plen - 1;
  float lat = 0, lon = 0;
  char tbl = 0, code = 0;
  bool ok = false;
  int commentOff = -1;  // offset into payload
  char name[10] = {0};
  int used = 0;

  if (type == '`' || type == '\'') {
    ok = parseMicE(dst, (const unsigned char*)body, blen, lat, lon, tbl, code);
    if (ok) commentOff = 1 + micEBodyOffset((const unsigned char*)body, blen);
  } else if (type == '!' || type == '=' || type == '/' || type == '@') {
    size_t off = 1;
    if ((type == '/' || type == '@') && blen >= 7) { body += 7; blen -= 7; off += 7; }
    ok = parsePos(body, blen, lat, lon, tbl, code, used);
    if (ok) commentOff = (int)off + used;
  } else if (type == ';') {  // object: 9-char name, '*'/'_', 7-char timestamp, position
    int ne = -1;
    for (int i = 0; i < (int)blen && i < 15; i++) if (body[i] == '*' || body[i] == '_') { ne = i; break; }
    if (ne >= 0 && ne + 8 < (int)blen) {
      ok = parsePos(body + ne + 8, blen - (ne + 8), lat, lon, tbl, code, used);
      if (ok) commentOff = 1 + ne + 8 + used;
      int n = ne < 9 ? ne : 9;
      memcpy(name, body, n); name[n] = 0;
    }
  } else if (type == ')') {  // item: name ends with '!' or '_'
    int ne = -1;
    for (int i = 0; i < 9 && i < (int)blen; i++) if (body[i] == '!' || body[i] == '_') { ne = i; break; }
    if (ne >= 0 && ne + 1 < (int)blen) {
      ok = parsePos(body + ne + 1, blen - (ne + 1), lat, lon, tbl, code, used);
      if (ok) commentOff = 1 + ne + 1 + used;
      memcpy(name, body, ne); name[ne] = 0;
    }
  }
  if (!ok || !latLonOk(lat, lon)) return false;

  st.lat = lat; st.lon = lon; st.hasLatLon = true;
  st.sym[0] = tbl ? tbl : ' ';
  st.sym[1] = code ? code : ' ';
  st.isWx = code == '_';
  if (name[0]) {  // object/item name replaces the source call, trimmed
    int n = (int)strlen(name);
    while (n > 0 && name[n - 1] == ' ') name[--n] = 0;
    const char* nm = name;
    while (*nm == ' ') nm++;
    if (*nm) { strncpy(st.call, nm, sizeof(st.call) - 1); st.call[sizeof(st.call) - 1] = 0; }
  }
  if (commentOff > 0 && (size_t)commentOff < plen) {
    sanitizeComment(payload + commentOff, st.comment, sizeof(st.comment));
    aprsExtractFreq(st.comment, st.freqMhz);
  }
  return true;
}
