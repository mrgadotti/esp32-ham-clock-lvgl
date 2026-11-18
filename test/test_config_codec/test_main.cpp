#include <string>
#include <string.h>
#include <unity.h>

#include "config_codec.h"

void setUp() {}
void tearDown() {}

static Config defaults() {
  Config c;
  configDefaults(c);
  return c;
}

void test_defaults_match_original() {
  Config c = defaults();
  TEST_ASSERT_EQUAL_STRING("dxcluster.iu1bow.it", c.cluster_host);
  TEST_ASSERT_EQUAL_INT(7300, c.cluster_port);
  TEST_ASSERT_EQUAL_STRING("rotate.aprs2.net", c.aprs_host);
  TEST_ASSERT_EQUAL_INT(14580, c.aprs_port);
  TEST_ASSERT_EQUAL_INT(10, c.aprs_radius);
  TEST_ASSERT_EQUAL_INT(29, c.aprs_int_min);
  TEST_ASSERT_EQUAL_UINT8(0, c.tft_lang);
  TEST_ASSERT_EQUAL_INT(100, c.tft_backlight);
  TEST_ASSERT_EQUAL_INT(30, c.tft_sw_sec);
  TEST_ASSERT_EQUAL_INT(60, c.alrt_scr_s);  // deliberate deviation from the original 5 s
  TEST_ASSERT_EQUAL_STRING("0", c.aprs_passcode);
  TEST_ASSERT_EQUAL_HEX32(0x13FE, c.scr_en);
  TEST_ASSERT_EQUAL_STRING("1234567890", c.ap_pass);
  const uint8_t order[12] = {1, 2, 6, 9, 4, 3, 5, 12, 7, 8, 0, 0};
  TEST_ASSERT_EQUAL_MEMORY(order, c.screen_order, 12);
}

void test_get_masks_secrets() {
  Config c = defaults();
  strcpy(c.wifi_pass, "hunter2");
  char buf[4096];
  TEST_ASSERT_TRUE(configToJson(c, buf, sizeof buf));
  TEST_ASSERT_NULL(strstr(buf, "\"wifi_pass\""));
  TEST_ASSERT_NULL(strstr(buf, "hunter2"));
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"wifi_pass_set\":true"));
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"wifi_pass2_set\":false"));
}

void test_get_fails_when_buffer_too_small() {
  Config c = defaults();
  char buf[32];
  TEST_ASSERT_FALSE(configToJson(c, buf, sizeof buf));
}

void test_roundtrip() {
  Config a = defaults();
  strcpy(a.user_callsign, "PY2XX");
  a.timezone = -3;
  a.tft_lang = 1;
  a.screen_order[3] = 9;
  char buf[4096];
  TEST_ASSERT_TRUE(configToJson(a, buf, sizeof buf));
  Config b = defaults();
  ApplyResult r = configApplyJson(b, buf);
  TEST_ASSERT_TRUE_MESSAGE(r.ok, r.error);
  TEST_ASSERT_EQUAL_STRING("PY2XX", b.user_callsign);
  TEST_ASSERT_EQUAL_FLOAT(-3, b.timezone);
  TEST_ASSERT_EQUAL_UINT8(1, b.tft_lang);
  TEST_ASSERT_EQUAL_UINT8(9, b.screen_order[3]);
}

void test_backup_has_plain_secrets_and_roundtrips() {
  Config a = defaults();
  strcpy(a.wifi_pass, "p\"w1");
  strcpy(a.wifi_pass2, "pw2");
  strcpy(a.ap_pass, "apsecret99");
  strcpy(a.hama_pass, "ham");
  strcpy(a.aprs_passcode, "12345");
  strcpy(a.qrz_pass, "qrz");
  strcpy(a.weather_key, "owkey");
  strcpy(a.user_callsign, "PY2XX");
  char buf[4096], plain[4096];
  TEST_ASSERT_TRUE(configToBackupJson(a, buf, sizeof buf));
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"ap_pass\":\"apsecret99\""));
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"weather_key\":\"owkey\""));
  TEST_ASSERT_NULL(strstr(buf, "_set\""));
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"user_callsign\":\"PY2XX\""));
  TEST_ASSERT_TRUE(configToJson(a, plain, sizeof plain));
  TEST_ASSERT_NULL(strstr(plain, "apsecret99"));  // GET /api/config unchanged
  Config b = defaults();
  ApplyResult r = configApplyJson(b, buf);
  TEST_ASSERT_TRUE_MESSAGE(r.ok, r.error);
  TEST_ASSERT_TRUE(r.needsRestart);
  TEST_ASSERT_EQUAL_STRING("p\"w1", b.wifi_pass);
  TEST_ASSERT_EQUAL_STRING("pw2", b.wifi_pass2);
  TEST_ASSERT_EQUAL_STRING("apsecret99", b.ap_pass);
  TEST_ASSERT_EQUAL_STRING("ham", b.hama_pass);
  TEST_ASSERT_EQUAL_STRING("12345", b.aprs_passcode);
  TEST_ASSERT_EQUAL_STRING("qrz", b.qrz_pass);
  TEST_ASSERT_EQUAL_STRING("owkey", b.weather_key);
  TEST_ASSERT_EQUAL_STRING("PY2XX", b.user_callsign);
}

void test_backup_fails_when_buffer_too_small() {
  Config c = defaults();
  char buf[32];
  TEST_ASSERT_FALSE(configToBackupJson(c, buf, sizeof buf));
}

void test_partial_update_keeps_absent() {
  Config c = defaults();
  strcpy(c.wifi_ssid, "home");
  ApplyResult r = configApplyJson(c, "{\"user_callsign\":\"PY2XX\"}");
  TEST_ASSERT_TRUE(r.ok);
  TEST_ASSERT_EQUAL_STRING("PY2XX", c.user_callsign);
  TEST_ASSERT_EQUAL_STRING("home", c.wifi_ssid);
}

void test_empty_secret_keeps() {
  Config c = defaults();
  strcpy(c.wifi_pass, "keepme");
  TEST_ASSERT_TRUE(configApplyJson(c, "{\"wifi_pass\":\"\"}").ok);
  TEST_ASSERT_EQUAL_STRING("keepme", c.wifi_pass);
  TEST_ASSERT_TRUE(configApplyJson(c, "{\"wifi_pass\":\"new\"}").ok);
  TEST_ASSERT_EQUAL_STRING("new", c.wifi_pass);
}

void test_clamps() {
  Config c = defaults();
  TEST_ASSERT_TRUE(configApplyJson(c, "{\"aprs_radius\":0,\"tft_backlight\":0,\"tft_switch_time_sec\":99999}").ok);
  TEST_ASSERT_EQUAL_INT(1, c.aprs_radius);
  TEST_ASSERT_TRUE(configApplyJson(c, "{\"aprs_radius\":500}").ok);
  TEST_ASSERT_EQUAL_INT(150, c.aprs_radius);
  TEST_ASSERT_TRUE(configApplyJson(c, "{\"aprs_radius\":50}").ok);
  TEST_ASSERT_EQUAL_INT(50, c.aprs_radius);
  TEST_ASSERT_EQUAL_INT(5, c.tft_backlight);
  TEST_ASSERT_EQUAL_INT(3600, c.tft_sw_sec);
}

void test_rejects_bad_json() {
  Config c = defaults();
  Config before = c;
  ApplyResult r = configApplyJson(c, "{not json");
  TEST_ASSERT_FALSE(r.ok);
  TEST_ASSERT_TRUE(r.error[0] != 0);
  TEST_ASSERT_EQUAL_MEMORY(&before, &c, sizeof c);
  // A wrong type mid-document must not leave earlier keys applied.
  r = configApplyJson(c, "{\"user_callsign\":\"X\",\"cluster_port\":\"abc\"}");
  TEST_ASSERT_FALSE(r.ok);
  TEST_ASSERT_EQUAL_MEMORY(&before, &c, sizeof c);
}

void test_needs_restart() {
  Config c = defaults();
  TEST_ASSERT_TRUE(configApplyJson(c, "{\"wifi_ssid\":\"x\"}").needsRestart);
  TEST_ASSERT_TRUE(configApplyJson(c, "{\"uart2_baud\":115200}").needsRestart);
  TEST_ASSERT_FALSE(configApplyJson(c, "{\"user_callsign\":\"PY2XX\"}").needsRestart);
  // Same value again is not a change.
  TEST_ASSERT_FALSE(configApplyJson(c, "{\"wifi_ssid\":\"x\"}").needsRestart);
}

void test_screen_bits_and_order() {
  Config c = defaults();
  TEST_ASSERT_TRUE(configApplyJson(c, "{\"scr_en\":65535,\"screen_order\":[1,1,11,14,2,12,0,0,13,3]}").ok);
  TEST_ASSERT_EQUAL_HEX32(0x1FFE, c.scr_en);
  const uint8_t want[12] = {1, 0, 11, 0, 2, 12, 0, 0, 0, 3, 0, 0};  // slots 0..9 from the JSON, dups and out-of-range (>12) reset to 0
  uint8_t tail[12];
  memcpy(tail, c.screen_order, 12);
  TEST_ASSERT_EQUAL_MEMORY(want, tail, 10);
  TEST_ASSERT_EQUAL_UINT8(0, tail[10]);
  TEST_ASSERT_EQUAL_UINT8(0, tail[11]);
}

void test_tft_rot_only_1_or_3() {
  Config c = defaults();
  TEST_ASSERT_TRUE(configApplyJson(c, "{\"tft_rotation\":3}").ok);
  TEST_ASSERT_EQUAL_INT(3, c.tft_rot);
  TEST_ASSERT_TRUE(configApplyJson(c, "{\"tft_rotation\":2}").ok);
  TEST_ASSERT_EQUAL_INT(1, c.tft_rot);
  TEST_ASSERT_TRUE(configApplyJson(c, "{\"tft_rotation\":0}").ok);
  TEST_ASSERT_EQUAL_INT(1, c.tft_rot);
}

void test_readonly_and_ascii() {
  Config c = defaults();
  TEST_ASSERT_TRUE(configApplyJson(c, "{\"user_lat_lon_valid\":true,\"aprs_comment\":\"caf\\u00e9\"}").ok);
  TEST_ASSERT_FALSE(c.user_ll_ok);
  TEST_ASSERT_EQUAL_STRING("caf??", c.aprs_comment);
}

void test_ssid_keeps_utf8() {
  Config c = defaults();
  TEST_ASSERT_TRUE(configApplyJson(c, "{\"wifi_ssid\":\"Caf\\u00e9\"}").ok);
  TEST_ASSERT_EQUAL_STRING("Caf\xc3\xa9", c.wifi_ssid);
}

void test_nvs_keys_fit() {
  for (size_t i = 0; i < kFieldCount; i++)
    if (kFields[i].nvsKey) TEST_ASSERT_TRUE_MESSAGE(strlen(kFields[i].nvsKey) <= 15, kFields[i].nvsKey);
}

// Patch applied in place: fields before a bad one (restart-flagged included) stay untouched, untouched fields keep
// their bytes, and the widest fields fit the scratch used by the in-place apply.
void test_in_place_apply() {
  Config c = defaults();
  Config before = c;
  ApplyResult r = configApplyJson(c, "{\"wifi_ssid\":\"x\",\"uart2_baud\":115200,\"tft_lang\":\"xx\"}");
  TEST_ASSERT_FALSE(r.ok);
  TEST_ASSERT_FALSE(r.needsRestart);
  TEST_ASSERT_EQUAL_MEMORY(&before, &c, sizeof c);

  std::string longStr(300, 'a');
  std::string patch = "{\"cluster_filters\":\"" + longStr + "\",\"aprs_alert\":\"" + longStr + "\",\"user_callsign\":\"PY2XX\"}";
  r = configApplyJson(c, patch.c_str());
  TEST_ASSERT_TRUE_MESSAGE(r.ok, r.error);
  TEST_ASSERT_EQUAL(sizeof c.cluster_filters - 1, strlen(c.cluster_filters));
  TEST_ASSERT_EQUAL(sizeof c.aprs_alert - 1, strlen(c.aprs_alert));
  TEST_ASSERT_EQUAL_STRING("PY2XX", c.user_callsign);
  TEST_ASSERT_EQUAL_STRING(before.cluster_host, c.cluster_host);
  TEST_ASSERT_EQUAL_INT(before.cluster_port, c.cluster_port);
}

void test_json_fields_fit_scratch() {
  for (size_t i = 0; i < kFieldCount; i++)
    if (kFields[i].jsonKey) TEST_ASSERT_TRUE_MESSAGE(kFields[i].size <= kMaxFieldSize, kFields[i].jsonKey);
}

// Every field set to a non-default, in-range value (config clamped so it is canonical), valid tcal.
static Config fullConfig() {
  Config a = defaults();
  uint8_t* base = (uint8_t*)&a;
  for (size_t i = 0; i < kFieldCount; i++) {
    const ConfigField& f = kFields[i];
    uint8_t* p = base + f.offset;
    switch (f.type) {
      case FT_STR: snprintf((char*)p, f.size, "v%u", (unsigned)i); break;
      case FT_INT: *(int32_t*)p = (int32_t)f.min + 1; break;
      case FT_FLT: *(float*)p = f.min + 1.25f; break;
      case FT_BOOL: *p = !*p; break;
      case FT_LANG: *p = 1; break;
      case FT_BYTES: break;
      case FT_BLOB: break;
    }
  }
  a.user_ll_ok = false;  // read-only on import
  a.tft_rot = 3;
  a.screen_order[0] = 9;
  a.screen_order[1] = 1;
  const float t[6] = {0.0888f, 0.001f, -17.5f, -0.002f, 0.0664f, -13.25f};
  memcpy(a.tcal, t, sizeof t);
  configClamp(a);
  return a;
}

void test_backup_has_every_key_tcal_and_last_scr() {
  Config a = fullConfig();
  char buf[6144];
  TEST_ASSERT_TRUE(configToBackupJson(a, buf, sizeof buf, 7));
  for (size_t i = 0; i < kFieldCount; i++) {
    TEST_ASSERT_NOT_NULL_MESSAGE(kFields[i].jsonKey, kFields[i].nvsKey);  // nothing may be left out of the backup
    std::string k = std::string("\"") + kFields[i].jsonKey + "\":";
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(buf, k.c_str()), kFields[i].jsonKey);
  }
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"tcal\":[0.0888"));
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"last_scr\":7"));
  char plain[6144];
  TEST_ASSERT_TRUE(configToJson(a, plain, sizeof plain));
  TEST_ASSERT_NULL(strstr(plain, "last_scr"));  // GET /api/config: no last_scr, secrets stay masked
  TEST_ASSERT_NULL(strstr(plain, "\"wifi_pass\":"));
}

void test_backup_roundtrip_reproduces_every_field() {
  Config a = fullConfig();
  char buf[6144];
  TEST_ASSERT_TRUE(configToBackupJson(a, buf, sizeof buf, 7));
  Config b = defaults();
  ApplyResult r = configApplyJson(b, buf);
  TEST_ASSERT_TRUE_MESSAGE(r.ok, r.error);
  TEST_ASSERT_TRUE(r.hasLastScr);
  TEST_ASSERT_EQUAL_UINT8(7, r.lastScr);
  TEST_ASSERT_TRUE(r.tcalSet);
  for (size_t i = 0; i < kFieldCount; i++) {
    const ConfigField& f = kFields[i];
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE((uint8_t*)&a + f.offset, (uint8_t*)&b + f.offset, f.size, f.nvsKey);
  }
}

void test_last_scr_normalised_and_validated() {
  Config a = defaults();
  char buf[6144];
  TEST_ASSERT_TRUE(configToBackupJson(a, buf, sizeof buf, 0));  // never stored
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"last_scr\":1"));
  for (const char* bad : {"{\"last_scr\":0}", "{\"last_scr\":13}", "{\"last_scr\":14}", "{\"last_scr\":300}", "{\"last_scr\":\"3\"}",
                          "{\"last_scr\":2.5}", "{\"last_scr\":-1}"}) {
    Config c = defaults();
    TEST_ASSERT_FALSE_MESSAGE(configApplyJson(c, bad).ok, bad);
  }
}

void test_removed_screens_cleared_by_clamp() {
  Config c = defaults();
  c.scr_en = 0xFFFF;
  const uint8_t o[12] = {13, 10, 11, 14, 2, 2, 0, 0, 0, 0, 0, 0};
  memcpy(c.screen_order, o, 12);
  configClamp(c);
  TEST_ASSERT_EQUAL_HEX32(0x1FFE, c.scr_en);
  TEST_ASSERT_EQUAL_UINT8(0, c.screen_order[0]);   // 13 dropped
  TEST_ASSERT_EQUAL_UINT8(10, c.screen_order[1]);  // Open-Meteo ids survive
  TEST_ASSERT_EQUAL_UINT8(11, c.screen_order[2]);
  TEST_ASSERT_EQUAL_UINT8(0, c.screen_order[3]);   // 14 dropped
  TEST_ASSERT_EQUAL_UINT8(2, c.screen_order[4]);
  TEST_ASSERT_EQUAL_UINT8(0, c.screen_order[5]);  // duplicate
}

void test_scr_en_clamp_keeps_open_meteo_bits() {
  Config c = defaults();
  c.scr_en = 0x3FFE;
  configClamp(c);
  TEST_ASSERT_EQUAL_HEX32(0x1FFE, c.scr_en);
  TEST_ASSERT_TRUE(configApplyJson(c, "{\"scr_en\":5118}").ok);  // old backup value 0x13FE still imports
  TEST_ASSERT_EQUAL_HEX32(0x13FE, c.scr_en);
}

void test_defaults_without_location() {
  Config c = defaults();
  TEST_ASSERT_EQUAL_HEX32(0x13FE, c.scr_en);
  const uint8_t order[12] = {1, 2, 6, 9, 4, 3, 5, 12, 7, 8, 0, 0};
  TEST_ASSERT_EQUAL_MEMORY(order, c.screen_order, 12);
}

void test_last_scr_13_rejected() {
  Config c = defaults();
  ApplyResult r = configApplyJson(c, "{\"last_scr\":13}");
  TEST_ASSERT_FALSE(r.ok);
  TEST_ASSERT_EQUAL_STRING("bad_value:last_scr", r.error);
}

void test_invalid_tcal_rejected_untouched() {
  const char* bad[] = {
      "{\"tcal\":[1,2,3]}", "{\"tcal\":[0,0,0,0,0,0]}", "{\"tcal\":\"x\"}", "{\"tcal\":[1,0,0,0,\"a\",0]}",
      "{\"tcal\":[50,0,0,0,50,0]}", "{\"tcal\":[0.1,0.1,0,0.2,0.2,0]}", "{\"tcal\":[0.1,0,1e9,0,0.1,0]}",
      "{\"user_callsign\":\"AA1BB\",\"tcal\":[0.1,0,0,0,0,0]}",
  };
  for (const char* j : bad) {
    Config c = fullConfig(), before = c;
    ApplyResult r = configApplyJson(c, j);
    TEST_ASSERT_FALSE_MESSAGE(r.ok, j);
    TEST_ASSERT_FALSE(r.tcalSet);
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(&before, &c, sizeof c, j);  // all-or-nothing, calibration kept
  }
}

void test_old_backup_and_unknown_keys() {
  Config c = defaults();
  c.tcal[0] = 0.5f;
  ApplyResult r = configApplyJson(c, "{\"user_callsign\":\"PY2XX\",\"future_key\":[1,2],\"another\":{\"x\":1}}");
  TEST_ASSERT_TRUE_MESSAGE(r.ok, r.error);
  TEST_ASSERT_FALSE(r.hasLastScr);
  TEST_ASSERT_FALSE(r.tcalSet);
  TEST_ASSERT_EQUAL_STRING("PY2XX", c.user_callsign);
  TEST_ASSERT_EQUAL_FLOAT(0.5f, c.tcal[0]);  // absent tcal keeps the stored one
}

void test_gps_module_migrates_to_none() {
  Config c = defaults();
  strcpy(c.uart2_cmod, "gps");
  configClamp(c);
  TEST_ASSERT_EQUAL_STRING("none", c.uart2_cmod);
  c = defaults();
  ApplyResult r = configApplyJson(c, "{\"uart2_connected_module\":\"gps\"}");
  TEST_ASSERT_TRUE_MESSAGE(r.ok, r.error);
  TEST_ASSERT_EQUAL_STRING("none", c.uart2_cmod);
}

void test_old_backup_gps_keys_ignored() {
  Config c = defaults();
  ApplyResult r = configApplyJson(c, "{\"use_gps_if_active\":true,\"uart2_mode\":\"gps\",\"user_callsign\":\"PY2XYZ\"}");
  TEST_ASSERT_TRUE_MESSAGE(r.ok, r.error);
  TEST_ASSERT_EQUAL_STRING("PY2XYZ", c.user_callsign);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_defaults_match_original);
  RUN_TEST(test_get_masks_secrets);
  RUN_TEST(test_get_fails_when_buffer_too_small);
  RUN_TEST(test_roundtrip);
  RUN_TEST(test_partial_update_keeps_absent);
  RUN_TEST(test_empty_secret_keeps);
  RUN_TEST(test_clamps);
  RUN_TEST(test_rejects_bad_json);
  RUN_TEST(test_needs_restart);
  RUN_TEST(test_screen_bits_and_order);
  RUN_TEST(test_tft_rot_only_1_or_3);
  RUN_TEST(test_readonly_and_ascii);
  RUN_TEST(test_ssid_keeps_utf8);
  RUN_TEST(test_backup_has_plain_secrets_and_roundtrips);
  RUN_TEST(test_backup_fails_when_buffer_too_small);
  RUN_TEST(test_nvs_keys_fit);
  RUN_TEST(test_in_place_apply);
  RUN_TEST(test_json_fields_fit_scratch);
  RUN_TEST(test_backup_has_every_key_tcal_and_last_scr);
  RUN_TEST(test_backup_roundtrip_reproduces_every_field);
  RUN_TEST(test_last_scr_normalised_and_validated);
  RUN_TEST(test_removed_screens_cleared_by_clamp);
  RUN_TEST(test_defaults_without_location);
  RUN_TEST(test_last_scr_13_rejected);
  RUN_TEST(test_invalid_tcal_rejected_untouched);
  RUN_TEST(test_old_backup_and_unknown_keys);
  RUN_TEST(test_scr_en_clamp_keeps_open_meteo_bits);
  RUN_TEST(test_gps_module_migrates_to_none);
  RUN_TEST(test_old_backup_gps_keys_ignored);
  return UNITY_END();
}
