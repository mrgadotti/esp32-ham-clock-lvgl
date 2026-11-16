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
  TEST_ASSERT_EQUAL_UINT8(LANG_EN, c.tft_lang);
  TEST_ASSERT_EQUAL_INT(100, c.tft_backlight);
  TEST_ASSERT_EQUAL_INT(30, c.tft_sw_sec);
  TEST_ASSERT_EQUAL_STRING("0", c.aprs_passcode);
  TEST_ASSERT_EQUAL_INT(0xFFF, c.scr_en);
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
  a.tft_lang = LANG_PT_BR;
  a.screen_order[3] = 9;
  char buf[4096];
  TEST_ASSERT_TRUE(configToJson(a, buf, sizeof buf));
  Config b = defaults();
  ApplyResult r = configApplyJson(b, buf);
  TEST_ASSERT_TRUE_MESSAGE(r.ok, r.error);
  TEST_ASSERT_EQUAL_STRING("PY2XX", b.user_callsign);
  TEST_ASSERT_EQUAL_FLOAT(-3, b.timezone);
  TEST_ASSERT_EQUAL_UINT8(LANG_PT_BR, b.tft_lang);
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

void test_nvs_keys_fit() {
  for (size_t i = 0; i < kFieldCount; i++)
    if (kFields[i].nvsKey) TEST_ASSERT_TRUE_MESSAGE(strlen(kFields[i].nvsKey) <= 15, kFields[i].nvsKey);
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
  RUN_TEST(test_nvs_keys_fit);
  return UNITY_END();
}
