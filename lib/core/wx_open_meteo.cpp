#include "wx_open_meteo.h"
#include <math.h>
#include <string.h>

int wmoToOwmId(int wmo) {
  switch (wmo) {
    case 0: return 800; case 1: return 801; case 2: return 802; case 3: return 804;
    case 45: case 48: return 741;
    case 51: return 300; case 53: return 301; case 55: return 302;
    case 56: case 57: case 66: case 67: return 511;
    case 61: return 500; case 63: return 501; case 65: return 502;
    case 71: case 77: return 600; case 73: return 601; case 75: return 602;
    case 80: return 520; case 81: return 521; case 82: return 522;
    case 85: return 620; case 86: return 621;
    case 95: return 211; case 96: case 99: return 202; case 97: return 212;
    default: return 800;
  }
}

Tr wxOmDescTr(int id) {
  if (id == 800) return Tr::WxCondClear;
  if (id == 801 || id == 802) return Tr::WxCondPartly;
  if (id == 803 || id == 804) return Tr::WxCondOvercast;
  if (id == 741) return Tr::WxCondFog;
  if (id == 511) return Tr::WxCondFreezingRain;
  if (id >= 520 && id < 540) return Tr::WxCondShowers;
  if (id >= 300 && id < 400) return Tr::WxCondDrizzle;
  if (id >= 500 && id < 600) return Tr::WxCondRain;
  if (id >= 600 && id < 700) return Tr::WxCondSnow;
  if (id >= 200 && id < 300) return Tr::WxCondThunder;
  return Tr::WxCondUnknown;
}

static bool has(JsonVariantConst a, int i) { return a.is<JsonArrayConst>() && !a[i].isNull(); }

bool openMeteoParse(JsonDocument& doc, WeatherData& out, bool airOnly) {
  if (!doc.is<JsonObject>()) return false;
  JsonVariantConst root = doc, cur = root["current"];
  if (root["error"] | false) return false;
  if (airOnly) {
    if (cur["pm2_5"].isNull() && cur["pm10"].isNull()) return false;
    out.pm25 = cur["pm2_5"] | 0.0f;
    out.pm10 = cur["pm10"] | 0.0f;
    return true;
  }
  if (!cur["temperature_2m"].is<float>()) return false;

  WeatherData t = out;  // commit only at the end
  t.tempC = cur["temperature_2m"] | 0.0f;
  t.humidity = (int)lroundf(cur["relative_humidity_2m"] | 0.0f);
  t.pressure = (int)lroundf(cur["surface_pressure"] | 0.0f);
  t.windMs = cur["wind_speed_10m"] | 0.0f;
  t.weatherId = wmoToOwmId(cur["weather_code"] | 0);
  t.day = (cur["is_day"] | 1) ? WxDay::Day : WxDay::Night;

  JsonVariantConst h = root["hourly"], d = root["daily"];
  // Columns 0..1: hourly +3h / +6h; columns 2..3: daily tomorrow / day after. Column 4 is unused here.
  const int hIdx[2] = {3, 6};
  for (int s = 0; s < 4; s++) {
    const bool hourly = s < 2;
    const int i = hourly ? hIdx[s] : s - 1;
    JsonVariantConst tmp = hourly ? h["temperature_2m"] : d["temperature_2m_max"];
    t.detailValid[s] = has(tmp, i);
    if (!t.detailValid[s]) continue;
    t.detailTempC[s] = tmp[i] | 0.0f;
    t.detailHumidity[s] = (int)lroundf((hourly ? h["relative_humidity_2m"][i] : d["relative_humidity_2m_mean"][i]) | 0.0f);
    t.detailWindMs[s] = (hourly ? h["wind_speed_10m"][i] : d["wind_speed_10m_max"][i]) | 0.0f;
    t.detailWeatherId[s] = wmoToOwmId((hourly ? h : d)["weather_code"][i] | 0);
    t.detailNight[s] = false;  // hourly/daily is_day not requested
  }
  for (int n = 0; n < 2; n++) {
    JsonVariantConst m = d["temperature_2m_min"];
    t.nightTempValid[n] = has(m, n + 1);
    if (t.nightTempValid[n]) t.nightTempC[n] = m[n + 1] | 0.0f;
  }
  float wmax = 0, gmax = 0;
  for (JsonVariantConst v : h["wind_speed_10m"].as<JsonArrayConst>()) wmax = fmaxf(wmax, v | 0.0f);
  for (JsonVariantConst v : h["wind_gusts_10m"].as<JsonArrayConst>()) gmax = fmaxf(gmax, v | 0.0f);
  t.fcWindMaxMs = wmax; t.fcWindGustMs = gmax;
  t.valid = true;
  out = t;
  return true;
}
