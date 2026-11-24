#pragma once
#include <stddef.h>
#include <stdint.h>

// Every persisted setting. Member names are the NVS keys (<= 15 chars); JSON names are in kFields.
struct Config {
  // WiFi
  char wifi_ssid[33], wifi_pass[65], wifi_ssid2[33], wifi_pass2[65], ap_pass[65];
  // Station
  char user_callsign[16], user_locator[11];
  float user_lat, user_lon, timezone;
  bool user_ll_ok;
  // DX cluster
  char cluster_host[64], cluster_filters[128], pota_api_url[96];
  int32_t cluster_port;
  bool cluster_noann, cluster_nowwv, cluster_nowcy, cluster_usefl;
  // HamAlert
  char hama_host[64], hama_user[32], hama_pass[32];
  int32_t hama_port;
  // APRS-IS and beacon
  char aprs_host[64], aprs_callsign[12], aprs_passcode[8], aprs_symbol[3], aprs_comment[44];
  int32_t aprs_port, aprs_ssid, aprs_radius, aprs_int_min;
  bool aprs_beacon;
  // Alerts
  char aprs_alert[128];
  bool aprs_alert_en, aprs_al_near, alrt_wx_en, wx_al_st, wx_al_wi, led_al_en;
  int32_t aprs_alrt_sec, alrt_scr_s, led_al_dur, led_al_blk;
  float alrt_dst_km, wx_w_thr;
  // Third-party credentials
  char qrz_user[32], qrz_pass[32], weather_key[40];
  // UART2 / bridge
  char uart2_cmod[20], uart2_par[6], br_pair_addr[13], br_pair_name[41];
  int32_t uart2_baud, uart2_dbits, uart2_stop;
  // Display / screens
  // tft_rot is 1 or 3 only. scr_en: bit index = ScreenId (bit 1 = HamClock ...);
  // only bits 1..13 are kept (10/11 = Open-Meteo screens), default 0x13FE. screen_order slots hold a ScreenId or 0 (off);
  // invalid or duplicate ids are reset to 0.
  int32_t tft_backlight, tft_ndim, tft_rot, tft_sw_sec, scr_en;
  int32_t screen1_time, s2_mode, s2_band, s7_mode, s7_band, s8_mode, s8_band, aprs_sort;
  bool tft_inv, tft_autosw;
  uint8_t tft_lang;  // 0 = EN, 1 = PT_BR (i18n Lang values)
  uint8_t screen_order[13];
  float tcal[6];  // touch calibration (TouchCal layout); all zero = not calibrated
};

enum FieldType : uint8_t { FT_STR, FT_INT, FT_FLT, FT_BOOL, FT_LANG, FT_BYTES, FT_BLOB };

struct ConfigField {
  const char* jsonKey;  // nullptr = not exposed over JSON
  const char* nvsKey;
  FieldType type;
  uint16_t offset, size;  // into Config; size in bytes (strings include the terminator)
  float min, max;         // INT/FLT clamp; BYTES element range
  bool isSecret, needsRestart;
};

extern const ConfigField kFields[];
extern const size_t kFieldCount;
