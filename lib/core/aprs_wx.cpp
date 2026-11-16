#include "aprs_wx.h"
#include <string.h>

static bool readDigits(const char* t, int len, int start, int n, int& out) {
  if (start + n > len) return false;
  int v = 0;
  for (int i = 0; i < n; i++) {
    char c = t[start + i];
    if (c < '0' || c > '9') return false;
    v = v * 10 + (c - '0');
  }
  out = v;
  return true;
}

bool aprsDecodeWx(const char* comment, AprsWx& wx) {
  wx = AprsWx();
  if (!comment) return false;
  while (*comment == ' ') comment++;
  if (*comment == '_') comment++;  // positionless WX marker (the original fails to decode these)
  int len = (int)strlen(comment);
  while (len > 0 && comment[len - 1] == ' ') len--;
  if (len == 0) return false;

  int dir = 0, mph = 0;
  if (len >= 7 && readDigits(comment, len, 0, 3, dir) && comment[3] == '/' && readDigits(comment, len, 4, 3, mph)) {
    wx.hasWindDir = true; wx.windDirDeg = dir % 360;
    wx.hasWindSpeed = true; wx.windSpeedKmh = mph * 1.60934f;
  }
  for (int i = 0; i < len; i++) {
    char tag = comment[i];
    int digits = 0;
    switch (tag) {
      case 'c': case 's': case 'g': case 't': case 'r': case 'p': case 'P': digits = 3; break;
      case 'h': digits = 2; break;
      case 'b': digits = 5; break;
      default: break;
    }
    int v = 0;
    if (!digits || !readDigits(comment, len, i + 1, digits, v)) continue;
    if (tag == 'c') { wx.hasWindDir = true; wx.windDirDeg = v % 360; }
    else if (tag == 's' || (tag == 'g' && !wx.hasWindSpeed)) { wx.hasWindSpeed = true; wx.windSpeedKmh = v * 1.60934f; }
    else if (tag == 't') { wx.hasTempC = true; wx.tempC = (v - 32.0f) * (5.0f / 9.0f); }
    else if (tag == 'h') { wx.hasHumidity = true; wx.humidityPct = v == 0 ? 100 : (v > 100 ? 100 : v); }
    else if (tag == 'b') { wx.hasPressure = true; wx.pressureHpa = v / 10.0f; }
  }
  wx.hasAny = wx.hasWindDir || wx.hasWindSpeed || wx.hasTempC || wx.hasHumidity || wx.hasPressure;
  return wx.hasAny;
}
