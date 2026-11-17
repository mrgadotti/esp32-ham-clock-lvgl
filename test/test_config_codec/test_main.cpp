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
  TEST_ASSERT_EQUAL_STRING("dxspots.com", c.cluster_host);
  TEST_ASSERT_EQUAL_INT(7300, c.cluster_port);
  TEST_ASSERT_EQUAL_STRING("rotate.aprs2.net", c.aprs_host);
  TEST_ASSERT_EQUAL_INT(14580, c.aprs_port);
  TEST_ASSERT_EQUAL_INT(50, c.aprs_radius);
  TEST_ASSERT_EQUAL_INT(29, c.aprs_int_min);
  TEST_ASSERT_EQUAL_UINT8(0, c.tft_lang);
  TEST_ASSERT_EQUAL_INT(100, c.tft_backlight);
  TEST_ASSERT_EQUAL_INT(30, c.tft_sw_sec);
  TEST_ASSERT_EQUAL_STRING("0", c.aprs_passcode);
  TEST_ASSERT_EQUAL_HEX32(0x33FE, c.scr_en);
  TEST_ASSERT_EQUAL_STRING("1234567890", c.ap_pass);
  const uint8_t order[12] = {1, 2, 6, 9, 4, 3, 5, 12, 7, 8, 13, 0};
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
  TEST_ASSERT_EQUAL_HEX32(0x33FE, c.scr_en);
  const uint8_t want[12] = {1, 0, 0, 0, 2, 12, 0, 0, 13, 3, 0, 0};  // slots 0..9 from the JSON, dups and out-of-range reset to 0
  uint8_t tail[12];
  memcpy(tail, c.screen_order, 12);
  TEST_ASSERT_EQUAL_MEMORY(want, tail, 10);
  TEST_ASSERT_EQUAL_UINT8(0, tail[10]);
  TEST_ASSERT_EQUAL_UINT8(0, tail[11]);  // default 13 duplicates slot 8, reset to 0
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
  RUN_TEST(test_nvs_keys_fit);
  RUN_TEST(test_in_place_apply);
  RUN_TEST(test_json_fields_fit_scratch);
  return UNITY_END();
}
