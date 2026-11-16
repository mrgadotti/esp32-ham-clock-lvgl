#include "config_codec.h"

#include <ArduinoJson.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#define OFS(m) (uint16_t) offsetof(Config, m)
#define SZ(m) (uint16_t) sizeof(((Config*)nullptr)->m)
// F(type, json, nvs, member, min, max, secret, restart); member name == nvs key.
#define F(t, j, m, lo, hi, sec, rst) {j, #m, t, OFS(m), SZ(m), lo, hi, sec, rst}

const ConfigField kFields[] = {
    // WiFi / AP (restart: radio is brought up once at boot)
    F(FT_STR, "wifi_ssid", wifi_ssid, 0, 0, false, true),
    F(FT_STR, "wifi_pass", wifi_pass, 0, 0, true, true),
    F(FT_STR, "wifi_ssid2", wifi_ssid2, 0, 0, false, true),
    F(FT_STR, "wifi_pass2", wifi_pass2, 0, 0, true, true),
    F(FT_STR, "ap_pass", ap_pass, 0, 0, true, true),
    // Station
    F(FT_STR, "user_callsign", user_callsign, 0, 0, false, false),
    F(FT_STR, "user_locator", user_locator, 0, 0, false, false),
    F(FT_FLT, "user_lat", user_lat, -90, 90, false, false),
    F(FT_FLT, "user_lon", user_lon, -180, 180, false, false),
    F(FT_BOOL, "user_lat_lon_valid", user_ll_ok, 0, 0, false, false),
    F(FT_FLT, "timezone", timezone, -12, 14, false, false),
    F(FT_BOOL, "use_gps_if_active", gps_auto_pos, 0, 0, false, false),
    // DX cluster
    F(FT_STR, "cluster_host", cluster_host, 0, 0, false, false),
    F(FT_INT, "cluster_port", cluster_port, 1, 65535, false, false),
    F(FT_BOOL, "cluster_noann", cluster_noann, 0, 0, false, false),
    F(FT_BOOL, "cluster_nowwv", cluster_nowwv, 0, 0, false, false),
    F(FT_BOOL, "cluster_nowcy", cluster_nowcy, 0, 0, false, false),
    F(FT_BOOL, "cluster_usefilters", cluster_usefl, 0, 0, false, false),
    F(FT_STR, "cluster_filters", cluster_filters, 0, 0, false, false),
    F(FT_STR, "pota_api_url", pota_api_url, 0, 0, false, false),
    // HamAlert
    F(FT_STR, "hamalert_host", hama_host, 0, 0, false, false),
    F(FT_INT, "hamalert_port", hama_port, 1, 65535, false, false),
    F(FT_STR, "hamalert_login", hama_user, 0, 0, false, false),
    F(FT_STR, "hamalert_password", hama_pass, 0, 0, true, false),
    // APRS-IS / beacon
    F(FT_STR, "aprs_host", aprs_host, 0, 0, false, false),
    F(FT_INT, "aprs_port", aprs_port, 1, 65535, false, false),
    F(FT_STR, "aprs_callsign", aprs_callsign, 0, 0, false, false),
    F(FT_STR, "aprs_passcode", aprs_passcode, 0, 0, true, false),
    F(FT_INT, "aprs_ssid", aprs_ssid, 0, 15, false, false),
    F(FT_INT, "aprs_radius", aprs_radius, 1, 50, false, false),
    F(FT_BOOL, "aprs_beacon", aprs_beacon, 0, 0, false, false),
    F(FT_STR, "aprs_symbol", aprs_symbol, 0, 0, false, false),
    F(FT_STR, "aprs_comment", aprs_comment, 0, 0, false, false),
    F(FT_INT, "aprs_interval_min", aprs_int_min, 1, 180, false, false),
    // Alerts
    F(FT_STR, "aprs_alert", aprs_alert, 0, 0, false, false),
    F(FT_BOOL, "aprs_alert_enabled", aprs_alert_en, 0, 0, false, false),
    F(FT_BOOL, "aprs_alert_nearby_enabled", aprs_al_near, 0, 0, false, false),
    F(FT_BOOL, "aprs_alert_wx_enabled", alrt_wx_en, 0, 0, false, false),
    F(FT_INT, "aprs_alert_min_sec", aprs_alrt_sec, 0, 86400, false, false),
    F(FT_INT, "aprs_alert_screen_sec", alrt_scr_s, 1, 60, false, false),
    F(FT_FLT, "aprs_alert_distance_km", alrt_dst_km, 0.1f, 500, false, false),
    F(FT_BOOL, "weather_alert_storm_enabled", wx_al_st, 0, 0, false, false),
    F(FT_BOOL, "weather_alert_wind_enabled", wx_al_wi, 0, 0, false, false),
    F(FT_FLT, "weather_alert_wind_threshold_ms", wx_w_thr, 1, 60, false, false),
    F(FT_BOOL, "enable_led_alert", led_al_en, 0, 0, false, false),
    F(FT_INT, "led_alert_duration_ms", led_al_dur, 100, 60000, false, false),
    F(FT_INT, "led_alert_blink_ms", led_al_blk, 50, 5000, false, false),
    // QRZ / OpenWeather
    F(FT_STR, "qrz_user", qrz_user, 0, 0, false, false),
    F(FT_STR, "qrz_pass", qrz_pass, 0, 0, true, false),
    F(FT_STR, "weather_key", weather_key, 0, 0, true, false),
    // UART2 / bridge
    F(FT_STR, "uart2_mode", uart2_mode, 0, 0, false, true),
    F(FT_STR, "uart2_connected_module", uart2_cmod, 0, 0, false, true),
    F(FT_INT, "uart2_baud", uart2_baud, 300, 2000000, false, true),
    F(FT_INT, "uart2_data_bits", uart2_dbits, 7, 8, false, true),
    F(FT_STR, "uart2_parity", uart2_par, 0, 0, false, true),
    F(FT_INT, "uart2_stop_bits", uart2_stop, 1, 2, false, true),
    F(FT_STR, nullptr, br_pair_addr, 0, 0, false, false),
    F(FT_STR, nullptr, br_pair_name, 0, 0, false, false),
    // Display / screens
    F(FT_INT, "tft_backlight", tft_backlight, 5, 100, false, false),
    F(FT_INT, "tft_night_dimming", tft_ndim, 0, 100, false, false),
    F(FT_BOOL, "tft_invert", tft_inv, 0, 0, false, false),
    F(FT_LANG, "tft_lang", tft_lang, 0, 0, false, false),
    F(FT_INT, "tft_rotation", tft_rot, 0, 3, false, false),
    F(FT_INT, "touch_rotation", touch_rot, 0, 3, false, false),
    F(FT_BOOL, "tft_auto_switch", tft_autosw, 0, 0, false, false),
    F(FT_INT, "tft_switch_time_sec", tft_sw_sec, 1, 3600, false, false),
    F(FT_BYTES, "screen_order", screen_order, 0, 13, false, false),
    F(FT_INT, "scr_en", scr_en, 0, 0xFFF, false, false),
    F(FT_INT, nullptr, screen1_time, 0, 1, false, false),
    F(FT_INT, nullptr, s2_mode, 0, 255, false, false),
    F(FT_INT, nullptr, s2_band, 0, 0xFFFF, false, false),
    F(FT_INT, nullptr, s7_mode, 0, 255, false, false),
    F(FT_INT, nullptr, s7_band, 0, 0xFFFF, false, false),
    F(FT_INT, nullptr, s8_mode, 0, 255, false, false),
    F(FT_INT, nullptr, s8_band, 0, 0xFFFF, false, false),
    F(FT_INT, nullptr, aprs_sort, 0, 255, false, false),
    F(FT_BLOB, nullptr, tcal, 0, 0, false, false),
};
const size_t kFieldCount = sizeof(kFields) / sizeof(kFields[0]);

static void setStr(char* dst, size_t size, const char* src) {
  snprintf(dst, size, "%s", src);
}

void configDefaults(Config& c) {
  memset(&c, 0, sizeof c);
  setStr(c.cluster_host, sizeof c.cluster_host, "dxspots.com");
  c.cluster_port = 7300;
  c.cluster_noann = c.cluster_nowwv = c.cluster_nowcy = c.cluster_usefl = true;
  setStr(c.cluster_filters, sizeof c.cluster_filters, "set/ft8");
  setStr(c.pota_api_url, sizeof c.pota_api_url, "https://api.pota.app/v1/spots");
  setStr(c.hama_host, sizeof c.hama_host, "hamalert.org");
  c.hama_port = 7300;
  setStr(c.aprs_host, sizeof c.aprs_host, "rotate.aprs2.net");
  c.aprs_port = 14580;
  setStr(c.aprs_callsign, sizeof c.aprs_callsign, "nocall");
  setStr(c.aprs_passcode, sizeof c.aprs_passcode, "0");
  c.aprs_radius = 50;
  c.aprs_beacon = true;
  setStr(c.aprs_symbol, sizeof c.aprs_symbol, "/V");
  c.aprs_int_min = 29;
  c.aprs_alert_en = c.aprs_al_near = c.led_al_en = true;
  c.aprs_alrt_sec = 300;
  c.alrt_scr_s = 5;
  c.alrt_dst_km = 1.0f;
  c.wx_w_thr = 12.0f;
  c.led_al_dur = 5000;
  c.led_al_blk = 500;
  setStr(c.uart2_mode, sizeof c.uart2_mode, "gps");
  setStr(c.uart2_cmod, sizeof c.uart2_cmod, "none");
  c.uart2_baud = 9600;
  c.uart2_dbits = 8;
  setStr(c.uart2_par, sizeof c.uart2_par, "none");
  c.uart2_stop = 1;
  c.tft_backlight = 100;
  c.tft_rot = 1;
  c.touch_rot = 1;
  c.tft_lang = LANG_EN;
  c.tft_sw_sec = 30;
  c.scr_en = 0xFFF;
  for (uint8_t i = 0; i < 12; i++) c.screen_order[i] = i;
}

static float clampf(float v, float lo, float hi) {
  if (!(v >= lo)) return lo;  // also catches NaN
  return v > hi ? hi : v;
}

void configClamp(Config& c) {
  uint8_t* base = (uint8_t*)&c;
  for (size_t i = 0; i < kFieldCount; i++) {
    const ConfigField& f = kFields[i];
    uint8_t* p = base + f.offset;
    switch (f.type) {
      case FT_STR: ((char*)p)[f.size - 1] = 0; break;
      case FT_INT: { int32_t* v = (int32_t*)p; *v = (int32_t)clampf((float)*v, f.min, f.max); break; }
      case FT_FLT: { float* v = (float*)p; *v = clampf(*v, f.min, f.max); break; }
      case FT_BOOL: *p = *p ? 1 : 0; break;
      case FT_LANG: if (*p > LANG_PT_BR) *p = LANG_EN; break;
      case FT_BYTES: for (uint16_t k = 0; k < f.size; k++) p[k] = (uint8_t)clampf(p[k], f.min, f.max); break;
      case FT_BLOB: break;
    }
  }
}

bool configToJson(const Config& c, char* buf, size_t len) {
  JsonDocument doc;
  const uint8_t* base = (const uint8_t*)&c;
  for (size_t i = 0; i < kFieldCount; i++) {
    const ConfigField& f = kFields[i];
    if (!f.jsonKey) continue;
    const uint8_t* p = base + f.offset;
    switch (f.type) {
      case FT_STR:
        if (f.isSecret) {
          char k[48];
          snprintf(k, sizeof k, "%s_set", f.jsonKey);
          doc[k] = ((const char*)p)[0] != 0;
        } else {
          doc[f.jsonKey] = (const char*)p;
        }
        break;
      case FT_INT: doc[f.jsonKey] = *(const int32_t*)p; break;
      case FT_FLT: doc[f.jsonKey] = *(const float*)p; break;
      case FT_BOOL: doc[f.jsonKey] = *(const bool*)p; break;
      case FT_LANG: doc[f.jsonKey] = *p == LANG_PT_BR ? "pt-br" : "en"; break;
      case FT_BYTES: {
        JsonArray a = doc[f.jsonKey].to<JsonArray>();
        for (uint16_t k = 0; k < f.size; k++) a.add(p[k]);
        break;
      }
      case FT_BLOB: break;
    }
  }
  if (doc.overflowed() || measureJson(doc) >= len) return false;
  serializeJson(doc, buf, len);
  return true;
}

static ApplyResult fail(const char* what, const char* key) {
  ApplyResult r = {false, false, {}};
  snprintf(r.error, sizeof r.error, "%s:%s", what, key ? key : "");
  return r;
}

ApplyResult configApplyJson(Config& cfg, const char* json) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) return fail("json_parse", nullptr);
  if (!doc.is<JsonObject>()) return fail("json_parse", nullptr);

  Config c = cfg;
  uint8_t* base = (uint8_t*)&c;
  for (size_t i = 0; i < kFieldCount; i++) {
    const ConfigField& f = kFields[i];
    if (!f.jsonKey) continue;
    JsonVariantConst v = doc[f.jsonKey];
    if (v.isNull()) continue;
    uint8_t* p = base + f.offset;
    switch (f.type) {
      case FT_STR: {
        if (!v.is<const char*>()) return fail("bad_type", f.jsonKey);
        const char* s = v.as<const char*>();
        while (isspace((unsigned char)*s)) s++;
        size_t n = strlen(s);
        while (n && isspace((unsigned char)s[n - 1])) n--;
        if (f.isSecret && n == 0) break;  // empty secret = keep
        if (n >= f.size) n = f.size - 1;
        memcpy(p, s, n);
        p[n] = 0;
        break;
      }
      case FT_INT:
        if (!v.is<double>()) return fail("bad_type", f.jsonKey);
        *(int32_t*)p = (int32_t)clampf((float)v.as<double>(), f.min, f.max);
        break;
      case FT_FLT:
        if (!v.is<double>()) return fail("bad_type", f.jsonKey);
        *(float*)p = clampf((float)v.as<double>(), f.min, f.max);
        break;
      case FT_BOOL:
        if (!v.is<bool>()) return fail("bad_type", f.jsonKey);
        *(bool*)p = v.as<bool>();
        break;
      case FT_LANG: {
        const char* s = v.is<const char*>() ? v.as<const char*>() : nullptr;
        if (s && !strcasecmp(s, "en")) *p = LANG_EN;
        else if (s && !strcasecmp(s, "pt-br")) *p = LANG_PT_BR;
        else return fail("bad_value", f.jsonKey);
        break;
      }
      case FT_BYTES: {
        if (!v.is<JsonArrayConst>()) return fail("bad_type", f.jsonKey);
        uint16_t k = 0;
        for (JsonVariantConst e : v.as<JsonArrayConst>()) {
          if (!e.is<double>()) return fail("bad_type", f.jsonKey);
          if (k < f.size) p[k++] = (uint8_t)clampf((float)e.as<double>(), f.min, f.max);
        }
        break;
      }
      case FT_BLOB: break;
    }
  }

  ApplyResult r = {true, false, {}};
  for (size_t i = 0; i < kFieldCount; i++) {
    const ConfigField& f = kFields[i];
    if (f.needsRestart && memcmp((uint8_t*)&c + f.offset, (const uint8_t*)&cfg + f.offset, f.size)) r.needsRestart = true;
  }
  cfg = c;
  return r;
}
