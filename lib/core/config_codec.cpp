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
    F(FT_INT, "tft_rotation", tft_rot, 1, 3, false, false),
    F(FT_INT, "touch_rotation", touch_rot, 0, 3, false, false),
    F(FT_BOOL, "tft_auto_switch", tft_autosw, 0, 0, false, false),
    F(FT_INT, "tft_switch_time_sec", tft_sw_sec, 1, 3600, false, false),
    F(FT_BYTES, "screen_order", screen_order, 0, 13, false, false),
    F(FT_INT, "scr_en", scr_en, 0, 0x3FFE, false, false),
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

static const uint32_t kScrEnMask = 0x33FE;  // ScreenId bits 1..13 except 10 (Matrix, removed) and 11 (Game, removed)
static const uint8_t kDefaultOrder[12] = {1, 2, 6, 9, 4, 3, 5, 12, 7, 8, 13, 0};

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
  c.tft_lang = 0;  // EN
  c.tft_sw_sec = 30;
  c.scr_en = kScrEnMask;
  memcpy(c.screen_order, kDefaultOrder, sizeof c.screen_order);
  setStr(c.ap_pass, sizeof c.ap_pass, "1234567890");
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
      case FT_LANG: if (*p > 1) *p = 0; break;
      case FT_BYTES: for (uint16_t k = 0; k < f.size; k++) if (p[k] > f.max) p[k] = 0; break;
      case FT_BLOB: break;
    }
  }
  c.scr_en &= kScrEnMask;
  if (c.tft_rot != 1 && c.tft_rot != 3) c.tft_rot = 1;
  uint32_t seen = 0;
  for (uint8_t& o : c.screen_order) {
    if (o > 13 || !((kScrEnMask | 1) >> o & 1) || (o && (seen >> o & 1))) o = 0;
    seen |= 1u << o;
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
      case FT_LANG: doc[f.jsonKey] = *p == 1 ? "pt-br" : "en"; break;
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

// Widest field (cluster_filters, aprs_alert): fields are parsed into this scratch, never into a second Config.
static const size_t kMaxFieldSize = 128;

// Parses v into p, a scratch copy of the field's current bytes. Returns the error kind, or nullptr on success.
static const char* parseField(const ConfigField& f, JsonVariantConst v, uint8_t* p) {
  switch (f.type) {
    case FT_STR: {
      if (!v.is<const char*>()) return "bad_type";
      const char* s = v.as<const char*>();
      while (isspace((unsigned char)*s)) s++;
      size_t n = strlen(s);
      while (n && isspace((unsigned char)s[n - 1])) n--;
      if (f.isSecret && n == 0) break;  // empty secret = keep
      if (n >= f.size) n = f.size - 1;
      memcpy(p, s, n);
      p[n] = 0;
      if (f.offset == OFS(wifi_ssid) || f.offset == OFS(wifi_ssid2)) {  // keep UTF-8 SSID bytes, drop control bytes
        size_t w = 0;
        for (size_t k = 0; k < n; k++)
          if (p[k] >= 0x20) p[w++] = p[k];
        p[w] = 0;
      } else if (!f.isSecret) {  // printable ASCII only (newline kept for the cluster filter list)
        for (size_t k = 0; k < n; k++)
          if (((char*)p)[k] != '\n' || f.offset != OFS(cluster_filters))
            if (p[k] < 0x20 || p[k] > 0x7E) p[k] = '?';
      }
      break;
    }
    case FT_INT:
      if (!v.is<double>()) return "bad_type";
      *(int32_t*)p = (int32_t)clampf((float)v.as<double>(), f.min, f.max);
      break;
    case FT_FLT:
      if (!v.is<double>()) return "bad_type";
      *(float*)p = clampf((float)v.as<double>(), f.min, f.max);
      break;
    case FT_BOOL:
      if (!v.is<bool>()) return "bad_type";
      *(bool*)p = v.as<bool>();
      break;
    case FT_LANG: {
      const char* s = v.is<const char*>() ? v.as<const char*>() : nullptr;
      if (s && !strcasecmp(s, "en")) *p = 0;
      else if (s && !strcasecmp(s, "pt-br")) *p = 1;
      else return "bad_value";
      break;
    }
    case FT_BYTES: {
      if (!v.is<JsonArrayConst>()) return "bad_type";
      uint16_t k = 0;
      for (JsonVariantConst e : v.as<JsonArrayConst>()) {
        if (!e.is<double>()) return "bad_type";
        double d = e.as<double>();
        if (k < f.size) p[k++] = (d >= f.min && d <= f.max) ? (uint8_t)d : 0;
      }
      break;
    }
    case FT_BLOB: break;
  }
  return nullptr;
}

// In place and all-or-nothing without a second Config: pass 0 only validates (parses each field into scratch and
// discards it), pass 1 commits the same parses, which cannot fail any more.
ApplyResult configApplyJson(Config& cfg, const char* json) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) return fail("json_parse", nullptr);
  if (!doc.is<JsonObject>()) return fail("json_parse", nullptr);

  uint8_t tmp[kMaxFieldSize];
  uint8_t* base = (uint8_t*)&cfg;
  ApplyResult r = {true, false, {}};
  for (int pass = 0; pass < 2; pass++) {
    for (size_t i = 0; i < kFieldCount; i++) {
      const ConfigField& f = kFields[i];
      if (!f.jsonKey || f.offset == OFS(user_ll_ok)) continue;  // user_lat_lon_valid is read-only
      JsonVariantConst v = doc[f.jsonKey];
      if (v.isNull()) continue;
      if (f.size > sizeof tmp) return fail("too_big", f.jsonKey);
      uint8_t* p = base + f.offset;
      memcpy(tmp, p, f.size);
      if (const char* err = parseField(f, v, tmp)) return fail(err, f.jsonKey);  // pass 0 only
      if (pass == 0) continue;
      if (f.needsRestart && memcmp(tmp, p, f.size)) r.needsRestart = true;
      memcpy(p, tmp, f.size);
    }
  }
  configClamp(cfg);
  return r;
}
