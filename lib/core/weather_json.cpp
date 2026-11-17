#include "weather_json.h"
#include <string.h>
#include <stdlib.h>

static void copyText(char* dst, size_t cap, const char* src) {
  size_t n = 0;
  for (const unsigned char* p = (const unsigned char*)(src ? src : ""); *p && n + 1 < cap; p++) {
    if (*p >= 0x20 && *p < 0x7F) dst[n++] = (char)*p;
    else if ((*p == 0xC2 || *p == 0xC3) && (p[1] & 0xC0) == 0x80) {
      // Keep only what the trimmed fonts carry: U+00B0 and U+00C0..U+00FF without U+00F7 (drops C1 controls and
      // U+00A0..U+00BF symbols).
      unsigned cp = ((*p & 0x1Fu) << 6) | (p[1] & 0x3Fu);
      if (cp != 0xB0 && (cp < 0xC0 || cp == 0xF7)) continue;
      if (n + 2 >= cap) break;
      dst[n++] = (char)*p; dst[n++] = (char)p[1]; p++;
    }  // else dropped (other lead bytes are dropped; their continuation bytes fail the test above)
  }
  dst[n] = 0;
}

bool isNightFromIcon(const char* c) {
  size_t n = c ? strlen(c) : 0;
  return n > 0 && c[n - 1] == 'n';
}

bool weatherParseCurrent(JsonDocument& doc, WeatherData& out) {
  if (!doc.is<JsonObject>()) return false;
  JsonVariantConst root = doc;  // const variant: one shared operator| per type instead of one per proxy chain
  if (!root["cod"].isNull() && root["cod"].as<int>() != 200) return false;
  JsonVariantConst m = root["main"], w = root["weather"][0];
  if (!m["temp"].is<float>() || w.isNull()) return false;
  copyText(out.description, sizeof out.description, w["description"] | "");
  if (!out.description[0]) strcpy(out.description, "--");
  copyText(out.cityName, sizeof out.cityName, root["name"] | "");
  copyText(out.iconCode, sizeof out.iconCode, w["icon"] | "");
  out.weatherId = w["id"] | 800;
  out.tempC = m["temp"] | 0.0f;
  out.humidity = m["humidity"] | 0;
  out.pressure = m["pressure"] | 0;
  out.windMs = root["wind"]["speed"] | 0.0f;
  out.valid = true;
  out.lastError[0] = 0;
  return true;
}

bool weatherParseAir(JsonDocument& doc, WeatherData& out) {
  JsonVariantConst c = JsonVariantConst(doc)["list"][0]["components"];
  if (c.isNull() || (c["pm2_5"].isNull() && c["pm10"].isNull())) return false;
  out.pm25 = c["pm2_5"] | 0.0f;
  out.pm10 = c["pm10"] | 0.0f;
  return true;
}

bool weatherParseForecast(JsonDocument& doc, WeatherData& out, long nowUnix, long tz) {
  if (!doc.is<JsonObject>()) return false;
  JsonVariantConst root = doc;
  if (!root["cod"].isNull() && root["cod"].as<int>() != 200) return false;
  JsonArrayConst list = root["list"].as<JsonArrayConst>();
  if (list.isNull() || list.size() == 0) return false;
  // Only object entries with a positive "dt" count; vi maps compact index -> list index.
  int vi[64], n = 0;
  for (size_t k = 0; k < list.size() && n < 64; k++)
    if (list[k].is<JsonObjectConst>() && (list[k]["dt"] | 0L) > 0) vi[n++] = (int)k;
  if (n == 0) return false;
  const bool haveClock = nowUnix > 100000L;

  // Next-6h storm / wind scan (feeds the alert scheduler).
  int stormEta = 0; float maxWind = 0, maxGust = 0;
  if (haveClock) {
    for (JsonVariantConst e : list) {
      long dt = e["dt"] | 0L, delta = dt - nowUnix;
      if (dt <= 0 || delta < 0 || delta > 6L * 3600L) continue;
      JsonVariantConst wind = e["wind"];
      float ws = wind["speed"] | 0.0f, g = wind["gust"] | 0.0f;
      if (ws > maxWind) maxWind = ws;
      if (g > maxGust) maxGust = g;
      int id = e["weather"][0]["id"] | 800;
      if (id >= 200 && id < 300) {
        int eta = (int)((delta + 59) / 60);
        if (eta < 1) eta = 1;
        if (stormEta == 0 || eta < stormEta) stormEta = eta;
      }
    }
  }

  const long targetOffsets[WX_DETAIL_COLS] = {3L * 3600, 6L * 3600, 24L * 3600, 48L * 3600, 72L * 3600};
  const int fallbackIdx[WX_DETAIL_COLS] = {0, 1, 7, 15, 23};
  const long baseDay = ((nowUnix + tz) / 86400L) * 86400L;

  WeatherData t = out;  // commit only at the end
  for (int s = 0; s < WX_DETAIL_COLS; s++) {
    int idx = -1;
    if (s >= 2 && haveClock) {
      const long dayStart = baseDay + (long)(s - 1) * 86400L, noon = dayStart + 12L * 3600L;
      long best = 0x7FFFFFFF;
      for (int i = 0; i < n; i++) {
        long dt = list[vi[i]]["dt"] | 0L;
        if (dt <= 0) continue;
        long l = dt + tz;
        if ((l / 86400L) * 86400L != dayStart) continue;
        int h = (int)((l % 86400L) / 3600L);
        const char* ic = list[vi[i]]["weather"][0]["icon"] | "";
        size_t il = strlen(ic);
        bool day = (h >= 9 && h <= 18) || (il && ic[il - 1] == 'd');
        long score = labs(l - noon) + (day ? 0 : 12L * 3600L);
        if (score < best) { best = score; idx = i; }
      }
    }
    if (idx < 0 && haveClock) {
      long target = nowUnix + targetOffsets[s], best = 0x7FFFFFFF;
      for (int i = 0; i < n; i++) {
        long dt = list[vi[i]]["dt"] | 0L;
        if (dt <= 0) continue;
        long d = labs(dt - target);
        if (d < best) { best = d; idx = i; }
      }
    }
    if (idx < 0) idx = fallbackIdx[s] < n ? fallbackIdx[s] : n - 1;
    JsonVariantConst e = list[vi[idx]], em = e["main"], ew = e["weather"][0];
    t.detailTempC[s] = em["temp"] | 0.0f;
    t.detailHumidity[s] = em["humidity"] | 0;
    t.detailWindMs[s] = e["wind"]["speed"] | 0.0f;
    t.detailWeatherId[s] = ew["id"] | 800;
    copyText(t.detailIconCode[s], sizeof t.detailIconCode[s], ew["icon"] | "");
    t.detailValid[s] = true;
  }

  // Night minimum for the next two days: entry closest to 02:00, else the day's coldest.
  for (int d = 0; d < 2; d++) {
    t.nightTempValid[d] = false;
    if (!haveClock) continue;
    const long dayStart = baseDay + (long)(d + 1) * 86400L;
    int bestNight = -1, bestAny = -1; long bestScore = 0x7FFFFFFF; float bestTemp = 1000.0f;
    for (int i = 0; i < n; i++) {
      long dt = list[vi[i]]["dt"] | 0L;
      if (dt <= 0) continue;
      long l = dt + tz;
      if ((l / 86400L) * 86400L != dayStart) continue;
      float temp = list[vi[i]]["main"]["temp"] | 0.0f;
      if (bestAny < 0 || temp < bestTemp) { bestTemp = temp; bestAny = i; }
      int h = (int)((l % 86400L) / 3600L);
      if (h > 6 && h < 21) continue;
      long score = labs((long)(h >= 21 ? h : 24 + h) - 26L);
      if (score < bestScore) { bestScore = score; bestNight = i; }
    }
    int pick = bestNight >= 0 ? bestNight : bestAny;
    if (pick >= 0) { t.nightTempC[d] = list[vi[pick]]["main"]["temp"] | 0.0f; t.nightTempValid[d] = true; }
  }

  t.forecast3hTempC = t.detailTempC[0]; t.forecast3hWindMs = t.detailWindMs[0];
  strcpy(t.forecast3hDesc, "--"); t.forecast3hValid = true;
  t.forecastNextDayTempC = t.detailTempC[2]; t.forecastNextDayWindMs = t.detailWindMs[2];
  strcpy(t.forecastNextDayDesc, "--"); t.forecastNextDayValid = true;
  t.forecastError[0] = 0;
  if (!t.cityName[0]) copyText(t.cityName, sizeof t.cityName, root["city"]["name"] | "");
  t.stormEtaMin = stormEta; t.fcWindMaxMs = maxWind; t.fcWindGustMs = maxGust;
  out = t;
  return true;
}
